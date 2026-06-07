cmake_minimum_required(VERSION 3.20)
project(ChessEngine CXX)

# Force C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src/Engine)

add_executable(ChessEngine src/main.cpp)

# Link the Engine library to main executable
target_link_libraries(ChessEngine PRIVATE Engine)