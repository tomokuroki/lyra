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
    "\"bus\": \"space\""
    "\"source\": \"drums\"")
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

# Prove that sidechain is audible, not metadata-only, by rendering the same
# source with only the sidechain command removed and comparing deterministic WAVs.
file(READ "${SOURCE_FILE}" SOURCE_TEXT)
string(REPLACE
    "sidechain drums amount=100 threshold=-30 ratio=8 attack=2 release=180"
    "# sidechain removed for verification"
    DRY_SOURCE "${SOURCE_TEXT}")
set(DRY_SOURCE_FILE "${OUTPUT_DIR}/dsp-graph-no-sidechain.lyra")
set(DRY_WAV_FILE "${OUTPUT_DIR}/dsp-graph-no-sidechain.wav")
file(WRITE "${DRY_SOURCE_FILE}" "${DRY_SOURCE}")
execute_process(COMMAND "${LYRA_EXE}" "${DRY_SOURCE_FILE}" "${DRY_WAV_FILE}"
    RESULT_VARIABLE DRY_RESULT OUTPUT_QUIET ERROR_VARIABLE ERROR_TEXT)
if(NOT DRY_RESULT EQUAL 0)
    message(FATAL_ERROR "Dry DSP graph WAV failed: ${ERROR_TEXT}")
endif()
file(SHA256 "${WAV_FILE}" WET_HASH)
file(SHA256 "${DRY_WAV_FILE}" DRY_HASH)
if(WET_HASH STREQUAL DRY_HASH)
    message(FATAL_ERROR "Sidechain did not change rendered audio")
endif()
