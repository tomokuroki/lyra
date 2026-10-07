if(NOT DEFINED LYRA_EXE OR NOT DEFINED SOURCE_FILE OR NOT DEFINED SOURCE_SONG OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "LYRA_EXE, SOURCE_FILE, SOURCE_SONG and OUTPUT_DIR are required")
endif()
set(JSON_FILE "${OUTPUT_DIR}/audio-clips-verification.json")
set(WAV_FILE "${OUTPUT_DIR}/audio-clips-verification.wav")
execute_process(COMMAND "${LYRA_EXE}" -f json "${SOURCE_FILE}" "${JSON_FILE}"
    RESULT_VARIABLE RESULT OUTPUT_QUIET ERROR_VARIABLE ERROR_TEXT)
if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "Audio clip JSON failed: ${ERROR_TEXT}")
endif()
file(READ "${JSON_FILE}" JSON_TEXT)
foreach(EXPECTED
    "\"length_beats\": 2"
    "\"trim_start_seconds\": 0.02"
    "\"pitch_semitones\": 3"
    "\"stretch\": 1.2"
    "\"loop\": true"
    "\"crossfade_ms\": 15"
    "\"reverse\": true")
    string(FIND "${JSON_TEXT}" "${EXPECTED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "Audio clip JSON is missing: ${EXPECTED}")
    endif()
endforeach()
execute_process(COMMAND "${LYRA_EXE}" "${SOURCE_FILE}" "${WAV_FILE}"
    RESULT_VARIABLE RESULT OUTPUT_QUIET ERROR_VARIABLE ERROR_TEXT)
if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "Audio clip WAV failed: ${ERROR_TEXT}")
endif()
file(SIZE "${WAV_FILE}" WAV_SIZE)
if(WAV_SIZE LESS 10000)
    message(FATAL_ERROR "Audio clip render is unexpectedly small")
endif()

# Exercise the native AIFF decoder with an asset produced by Lyra itself.
set(AIFF_FILE "${OUTPUT_DIR}/clip-source.aiff")
execute_process(COMMAND "${LYRA_EXE}" -f aiff "${SOURCE_SONG}" "${AIFF_FILE}"
    RESULT_VARIABLE AIFF_EXPORT_RESULT OUTPUT_QUIET ERROR_VARIABLE ERROR_TEXT)
if(NOT AIFF_EXPORT_RESULT EQUAL 0)
    message(FATAL_ERROR "AIFF fixture export failed: ${ERROR_TEXT}")
endif()
set(AIFF_PROJECT "${OUTPUT_DIR}/aiff-clip.lyra")
file(TO_CMAKE_PATH "${AIFF_FILE}" AIFF_PORTABLE)
file(WRITE "${AIFF_PROJECT}" "tempo 120\nsound modern\ntrack audio {\n  sample \"${AIFF_PORTABLE}\" 2\n}\n")
set(AIFF_WAV "${OUTPUT_DIR}/aiff-clip.wav")
execute_process(COMMAND "${LYRA_EXE}" "${AIFF_PROJECT}" "${AIFF_WAV}"
    RESULT_VARIABLE AIFF_RESULT OUTPUT_QUIET ERROR_VARIABLE ERROR_TEXT)
if(NOT AIFF_RESULT EQUAL 0)
    message(FATAL_ERROR "Native AIFF clip decode failed: ${ERROR_TEXT}")
endif()
file(SHA256 "${WAV_FILE}" TRANSFORMED_HASH)
file(SHA256 "${AIFF_WAV}" AIFF_HASH)
if(TRANSFORMED_HASH STREQUAL AIFF_HASH)
    message(FATAL_ERROR "Clip transformations did not affect PCM")
endif()
