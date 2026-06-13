# This creates a static library called "Engine"
add_library(Engine
        board.cpp
        board.h
        magicNumbers.h
        magicNumbers.cpp
        moveGeneration.cpp
        moveGeneration.h

)

# This allows main.cpp to #include "board.h"
target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})