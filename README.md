# C++ Chess Engine

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
