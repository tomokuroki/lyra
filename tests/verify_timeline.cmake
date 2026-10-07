if(NOT DEFINED LYRA_EXE OR NOT DEFINED SOURCE_FILE OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "LYRA_EXE, SOURCE_FILE and OUTPUT_DIR are required")
endif()

set(JSON_FILE "${OUTPUT_DIR}/timeline-verification.json")
set(MIDI_FILE "${OUTPUT_DIR}/timeline-verification.mid")

execute_process(
    COMMAND "${LYRA_EXE}" -f json "${SOURCE_FILE}" "${JSON_FILE}"
    RESULT_VARIABLE JSON_RESULT
    OUTPUT_QUIET ERROR_VARIABLE JSON_ERROR)
if(NOT JSON_RESULT EQUAL 0)
    message(FATAL_ERROR "Timeline JSON render failed: ${JSON_ERROR}")
endif()

file(READ "${JSON_FILE}" JSON_TEXT)
foreach(EXPECTED
    "\"beat\": 2, \"bpm\": 90"
    "\"name\": \"intro\""
    "\"pattern\": \"motif\""
    "\"start_beat\": 0.55")
    string(FIND "${JSON_TEXT}" "${EXPECTED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "Timeline JSON is missing: ${EXPECTED}")
    endif()
endforeach()

execute_process(
    COMMAND "${LYRA_EXE}" -f midi "${SOURCE_FILE}" "${MIDI_FILE}"
    RESULT_VARIABLE MIDI_RESULT
    OUTPUT_QUIET ERROR_VARIABLE MIDI_ERROR)
if(NOT MIDI_RESULT EQUAL 0)
    message(FATAL_ERROR "Timeline MIDI render failed: ${MIDI_ERROR}")
endif()
file(SIZE "${MIDI_FILE}" MIDI_SIZE)
if(MIDI_SIZE LESS 100)
    message(FATAL_ERROR "Timeline MIDI output is unexpectedly small")
endif()
