cmake_minimum_required(VERSION 3.10)
project(lyra VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lyra
    src/main.cpp
    src/parser.cpp
    src/synth.cpp
    src/export.cpp
    src/frontend.cpp
)

target_include_directories(lyra PRIVATE src)
