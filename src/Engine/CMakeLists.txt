# This creates a static library called "Engine"
add_library(Engine
        src/board.cpp
        src/magicNumbers.cpp
        src/moveGeneration.cpp
        include/board.h
        include/magicNumbers.h
        include/moveGeneration.h
        include/magics.h
)

# This allows main.cpp to #include "board.h"
target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})