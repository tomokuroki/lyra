cmake_minimum_required(VERSION 3.10)
project(lyra VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lyra
    src/main.cpp
    src/parser.cpp
    src/project.cpp
    src/dsp.cpp
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
    add_test(NAME lyra_timeline_render
        COMMAND "${CMAKE_COMMAND}"
            "-DLYRA_EXE=$<TARGET_FILE:lyra>"
            "-DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/examples/lyra3/project_ir.lyra"
            "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_timeline.cmake")
    add_test(NAME lyra_dsp_graph_render
        COMMAND "${CMAKE_COMMAND}"
            "-DLYRA_EXE=$<TARGET_FILE:lyra>"
            "-DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/examples/lyra3/dsp_graph.lyra"
            "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_dsp_graph.cmake")
    add_test(NAME lyra_synth_engine_render
        COMMAND "${CMAKE_COMMAND}"
            "-DLYRA_EXE=$<TARGET_FILE:lyra>"
            "-DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/examples/lyra3/synth_engine.lyra"
            "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_synth_engine.cmake")
    add_test(NAME lyra_generative_music_render
        COMMAND "${CMAKE_COMMAND}"
            "-DLYRA_EXE=$<TARGET_FILE:lyra>"
            "-DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/examples/lyra3/generative_music.lyra"
            "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_generative_music.cmake")
endif()
