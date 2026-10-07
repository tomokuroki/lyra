if(NOT DEFINED LYRA_EXE OR NOT DEFINED SOURCE_FILE OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "LYRA_EXE, SOURCE_FILE and OUTPUT_DIR are required")
endif()
set(JSON_FILE "${OUTPUT_DIR}/dsp-graph-verification.json")
set(WAV_FILE "${OUTPUT_DIR}/dsp-graph-verification.wav")
execute_process(COMMAND "${LYRA_EXE}" -f json "${SOURCE_FILE}" "${JSON_FILE}"
    RESULT_VARIABLE JSON_RESULT OUTPUT_QUIET ERROR_VARIABLE ERROR_TEXT)
if(NOT JSON_RESULT EQUAL 0)
    message(FATAL_ERROR "DSP graph JSON failed: ${ERROR_TEXT}")
endif()
file(READ "${JSON_FILE}" JSON_TEXT)
foreach(EXPECTED
    "\"id\": \"space\""
    "\"master_inserts\": [\"compressor\", \"limiter\"]"
    "\"inserts\": [\"highpass\", \"saturation\"]"
    "\"bus\": \"space\"")
    string(FIND "${JSON_TEXT}" "${EXPECTED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "DSP graph JSON is missing: ${EXPECTED}")
    endif()
endforeach()
execute_process(COMMAND "${LYRA_EXE}" "${SOURCE_FILE}" "${WAV_FILE}"
    RESULT_VARIABLE WAV_RESULT OUTPUT_QUIET ERROR_VARIABLE ERROR_TEXT)
if(NOT WAV_RESULT EQUAL 0)
    message(FATAL_ERROR "DSP graph WAV failed: ${ERROR_TEXT}")
endif()
file(SIZE "${WAV_FILE}" WAV_SIZE)
if(WAV_SIZE LESS 10000)
    message(FATAL_ERROR "DSP graph WAV output is unexpectedly small")
endif()
