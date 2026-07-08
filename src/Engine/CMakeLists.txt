# This creates a static library called "Engine"
add_library(Engine
        # Source Files
        src/board.cpp
        src/magicNumbers.cpp
        src/moveGeneration.cpp
        src/evaluation.cpp
        src/perft.cpp

        # Header files
        include/board.h
        include/magicNumbers.h
        include/moveGeneration.h
        include/magics.h
        include/move.h
        include/perft.h
        include/evaluation.h
        include/search.h
        src/search.cpp
        include/zobrist.h
        src/zobrist.cpp
        include/transposition.h
        src/transposition.cpp
        include/polyglot.h
        src/polyglot.cpp
)

# This allows main.cpp to #include "board.h"
target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})