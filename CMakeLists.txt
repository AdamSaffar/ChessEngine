cmake_minimum_required(VERSION 3.20)
project(ChessEngine CXX)

# Force C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch SFML directory
include(FetchContent)
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.1
)
FetchContent_MakeAvailable(SFML)

add_subdirectory(src/Engine)

# Terminal executable
add_executable(ChessCLI src/CLI/main.cpp)
target_link_libraries(ChessCLI PRIVATE Engine)

add_executable(ChessGUI src/GUI/main.cpp)
target_link_libraries(ChessGUI PRIVATE Engine sfml-graphics sfml-window sfml-system)