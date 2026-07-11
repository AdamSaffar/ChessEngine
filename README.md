# C++ Chess Engine

## Board Representation & Move Generation
A major design decision early on was opting to use bitboards rather than simplistic 2D arrays to represent the chessboard. The primary reason for this is performance at the memory level. In a standard 2D array, representing a single piece requires an entire byte (or even 4 bytes) just to store its piece identifier. With bitboards, that same piece is represented by a single bit inside a 64-bit integer. When the engine is evaluating millions of different board states, memory optimization matters immensely. Every fraction of a second the CPU cache misses and has to fetch bloated array data from RAM is time stolen from evaluating a crucial winning move.

This architecture also allows the engine to calculate legal moves, captures, and attacks using incredibly fast native bitwise operations (AND, OR, XOR, Bitshifts). For example, pushing all pawns one square forward is efficiently calculated using a left shift (<< 8). Complex sliding attacks are generated using local Look-Up Tables (LUTs). To ensure complete accuracy, move generation was validated using Perft (Performance Test) debugging. The engine successfully generates all pseudo-legal and strictly legal moves, matching benchmarks perfectly (e.g., exactly 119,060,324 positions at Depth 6, and 3,195,901,860 positions at Depth 7).  

## Search Algorithm & Memory
The core "brain" of the engine relies on the Negamax algorithm paired with Alpha-Beta Pruning. Searching the entire game tree blindly is computationally impossible. Alpha-Beta optimizes this by tracking the guaranteed minimum score the engine can force (Alpha) and the maximum score the opponent will allow (Beta). If a branch allows the opponent to drag the score below the engine's established safety net, the calculation is immediately abandoned, pruning millions of useless nodes.

Because chess trees heavily transpose (reaching the exact same board state through different move orders), the engine utilizes a Transposition Table (TT). We use Zobrist Hashing to generate a near-perfectly unique 64-bit identifier for any position. Transposition tables are incredibly efficient, acting as a local lookup table that compacts the following data into just two 64-bit integers:  
* Board state
* The evaluation score
* The best move
* Node flags
* Depth reached

One 64-bit integer represents the hash key, and the other represents the "data" payload. Before the engine starts a search, it performs a quick O(1) lookup in the TT. To verify it has found the exact position, it utilizes a XOR trick. The checksum is generated: checksum = zobristKey ^ data. Upon retrieval, if we XOR the checksum and the data (checksum ^ data), the result must exactly match the zobristKey. Given that a single TT entry is only about 16 bytes, allocating a reasonable 1 GB of memory allows the table to hold roughly 64,000,000 unique board positions at once.

## Algorithmic Pruning & Heuristics
Alpha-Beta pruning is only efficient if the engine evaluates the absolute best moves first. When the engine evaluates a strong move early, it immediately establishes a high Alpha; consequently, when evaluating worse moves later, the engine can instantly prune those branches since it has already proven it can force a better outcome. To achieve optimal move ordering, the engine relies on several advanced heuristics:  
* **Iterative Deepening:** The engine searches Depth 1, then Depth 2, then Depth 3, etc. This populates the Transposition Table with highly accurate principal variations (the sequence of moves both players are expected to make assuming optimal play), ensuring the deepest search always looks at the best moves first.
* **MVV-LVA (Most Valuable Victim - Least Valuable Attacker):** Captures are sorted to prioritize taking high-value pieces with low-value pieces. For instance, the highest-scoring move to check first would be a Pawn capturing a Queen (which is almost always advantageous).
* **Killer & History Heuristics:** Non-capturing quiet moves that frequently cause Alpha-Beta cutoffs in sibling branches are bumped to the front of the move list.
* **Principal Variation Search (PVS):** Assuming the first sorted move is the best, the engine searches it with a full window. It then uses a "Zero Window" on subsequent moves to cheaply prove they are worse.
* **Null Move Pruning (NMP):** The engine intentionally "skips" a turn. If the evaluation still beats Beta even after giving the opponent a free move, the position is overwhelmingly strong, and the branch is pruned entirely. On average, NMP reduced the nodes searched at Depth 8 from over 170 million down to the 16–41 million range.
* **Late Move Reductions (LMR):** Moves near the bottom of the rigorously sorted move list (essentially non-capture, non-promotion, non-killer, and non-history moves) are searched at a shallower depth to save CPU cycles.

## Concurrency: Lazy SMP
To drastically scale Nodes Per Second (NPS), the engine uses Lazy SMP (Symmetric Multiprocessing). Rather than explicitly dividing the search tree and incurring massive communication and lock overhead, Lazy SMP spins up multiple threads that search the exact same root position independently.

The threads communicate purely by reading and writing to the shared, lockless Transposition Table. If Thread A finds a brilliant move, it caches it; Thread B encounters the state a microsecond later, reads the cached evaluation, and explores a completely different branch. During testing, relying entirely on microscopic OS scheduling delays to separate the threads did not work as effectively as anticipated. To address this, I implemented an artificial jitter: when evaluating non-capture, non-killer quiet moves, the engine applies a randomized score between 0 and 15 points using a PRNG to break ties differently across threads.

This architecture allows the engine to scale with proper hardware(Tested with I7-12700, 20 threads). In testing, increasing from 1 thread to 12 threads boosted calculation speeds from ~1.47 Million NPS to ~19.6 Million NPS, crushing the time required to resolve Depth 10 from 189ms down to just 42ms. 

## External Data: Syzygy Tablebases & Polyglot Opening Books 
To optimize the early and late game, the engine connects to external databases. This beautiful combination of opening books and tablebases preserves the bulk of the engine's clock time for the most complicated portion of the game: the middlegame.
* **Polyglot Opening Book (.bin):** During the opening phase, the engine hashes the board and plays well-established main-line theory instantly.
* **Syzygy Tablebases:** Integrated via the Fathom C++ API, the engine utilizes Syzygy .rtbw and .rtbz files. Technically, Syzygy tablebases can evaluate positions with 7 or fewer pieces; however, the size of a complete 7-piece tablebase is an astonishing 18.4 Terabytes. Instead, this engine relies on a far more reasonable 3-4-5 piece tablebase, which is only about 1 GB. During endgames with 5 or fewer pieces, the engine queries the tablebase. Inside the tree, it acts as an absolute mathematical WDL (Win/Draw/Loss) evaluator to force Alpha-Beta cutoffs. At the root, it queries DTZ (Distance-to-Zero) tables to instantly play mathematically forced checkmates, entirely bypassing the search tree. 

## Performance & Tournament Results

The engine's evaluation method relies on piece-square tables, phase weights (interpolating between midgame and endgame), king safety structures, and piece mobility bonuses.  
Piece-square tables give a piece a fixed bonus or penalty depending on the square it is on. For example, a Knight on the edge of the board receives a penalty because it controls fewer squares and has less mobility; therefore, 
centralized Knights that occupy important squares are evaluated as more valuable. Phase weights work side-by-side with piece-square tables because, depending on the phase of the game, optimal piece placement shifts. During the middlegame, 
it is heavily advised for the King to be tucked safely away behind a wall of pawns. However, in an endgame scenario (when most pieces have been exchanged), it becomes highly advantageous to march the King toward the center of the board to actively participate. 
Piece mobility applies scaled bonuses based on how many squares a piece controls, and pawns are rewarded based on how far they have advanced toward a Queen promotion.  Finally, a dynamic time management system was implemented, replacing static depth limits with 
a modern soft/hard bound system to intelligently manage the clock.  To benchmark the engine, automated tournament testing was conducted via the Cute Chess CLI framework at a time control of 10s + 0.1s increments. All opposing engines were sourced from the extensive 
list at computerchess.org. 

| Opponent Engine | Opponent ELO | Score (W-L-D) | Win % | Games | ELO Difference | LOS | Draw Rate | Repository |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Vice 1.1** | 2000 | 209 - 55 - 36 | 75.7% | 300 | +197.1 ± 42.2 | 100.0% | 12.0% | [Link](https://github.com/bluefeversoft/vice) |
| **Kobol 1.0** | 2100 | 197 - 232 - 71 | 46.5% | 500 | -24.4 ± 28.3 | 4.6% | 14.2% | [Link](https://github.com/maxivolkov/kobol) |
| **KhepriChess 4.0.1** | 2200 | 297 - 145 - 58 | 65.2% | 500 | +109.1 ± 29.9 | 100.0% | 11.6% | [Link](https://github.com/kurt1288/KhepriChess) |
| **Napoleon** | 2300 | 157 - 199 - 144 | 45.8% | 500 | -29.3 ± 25.8 | 1.3% | 28.8% | [Link](https://github.com/crybot/Napoleon) |
| **fatalii** | 2400 | 64 - 381 - 55 | 18.3% | 500 | -259.9 ± 35.8 | 0.0% | 11.0% | [Link](https://github.com/FitzOReilly/fatalii) |

Using the weighted average of these performances against established benchmarks, the combined estimated Elo for my engine is **2198.6**.
