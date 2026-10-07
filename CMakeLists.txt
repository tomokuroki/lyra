cmake_minimum_required(VERSION 3.10)
project(lyra VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lyra
    src/main.cpp
    src/parser.cpp
    src/project.cpp
    src/synth.cpp
    src/export.cpp
    src/frontend.cpp
)

target_include_directories(lyra PRIVATE src)

if(MSVC)
    target_compile_options(lyra PRIVATE /EHsc)
endif()

include(CTest)
if(BUILD_TESTING)
    add_test(NAME lyra_regression_examples
        COMMAND lyra test "${CMAKE_CURRENT_SOURCE_DIR}/examples")
endif()
