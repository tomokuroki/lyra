if(NOT DEFINED LYRA_EXE OR NOT DEFINED SOURCE_FILE OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "LYRA_EXE, SOURCE_FILE and OUTPUT_DIR are required")
endif()
set(JSON_FILE "${OUTPUT_DIR}/synth-engine-verification.json")
set(WAV_FILE "${OUTPUT_DIR}/synth-engine-verification.wav")
execute_process(COMMAND "${LYRA_EXE}" -f json "${SOURCE_FILE}" "${JSON_FILE}"
    RESULT_VARIABLE JSON_RESULT OUTPUT_QUIET ERROR_VARIABLE ERROR_TEXT)
if(NOT JSON_RESULT EQUAL 0)
    message(FATAL_ERROR "Synth JSON failed: ${ERROR_TEXT}")
endif()
file(READ "${JSON_FILE}" JSON_TEXT)
foreach(EXPECTED
    "\"decay\": 0.25"
    "\"sustain\": 0.62"
    "\"unison\": 8"
    "\"fm_ratio\": 2"
    "\"am_rate\": 3.5"
    "\"wave\": \"saw\""
    "\"wave\": \"square\""
    "\"wave\": \"pink_noise\""
    "\"filter_type\": 2"
    "\"resonance\": 0.6"
    "\"pitch_envelope\": [12, 0]"
    "\"filter_envelope\": [350, 5200]"
    "\"lfo_count\": 3")
    string(FIND "${JSON_TEXT}" "${EXPECTED}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "Synth JSON is missing: ${EXPECTED}")
    endif()
endforeach()
execute_process(COMMAND "${LYRA_EXE}" "${SOURCE_FILE}" "${WAV_FILE}"
    RESULT_VARIABLE WAV_RESULT OUTPUT_QUIET ERROR_VARIABLE ERROR_TEXT)
if(NOT WAV_RESULT EQUAL 0)
    message(FATAL_ERROR "Synth WAV failed: ${ERROR_TEXT}")
endif()
file(READ "${SOURCE_FILE}" SOURCE_TEXT)
string(REPLACE "unison 8 detune=14" "unison 1 detune=0" MONO_SOURCE "${SOURCE_TEXT}")
set(MONO_FILE "${OUTPUT_DIR}/synth-engine-mono.lyra")
set(MONO_WAV "${OUTPUT_DIR}/synth-engine-mono.wav")
file(WRITE "${MONO_FILE}" "${MONO_SOURCE}")
execute_process(COMMAND "${LYRA_EXE}" "${MONO_FILE}" "${MONO_WAV}"
    RESULT_VARIABLE MONO_RESULT OUTPUT_QUIET ERROR_VARIABLE ERROR_TEXT)
if(NOT MONO_RESULT EQUAL 0)
    message(FATAL_ERROR "Mono synth WAV failed: ${ERROR_TEXT}")
endif()
file(SHA256 "${WAV_FILE}" UNISON_HASH)
file(SHA256 "${MONO_WAV}" MONO_HASH)
if(UNISON_HASH STREQUAL MONO_HASH)
    message(FATAL_ERROR "Unison did not change rendered audio")
endif()

string(REPLACE "lfo sine 5 pitch 0.18" "# lfo removed" NO_LFO_SOURCE "${SOURCE_TEXT}")
string(REPLACE "lfo triangle 0.4 cutoff 900" "# lfo removed" NO_LFO_SOURCE "${NO_LFO_SOURCE}")
string(REPLACE "lfo random 2 pan 30" "# lfo removed" NO_LFO_SOURCE "${NO_LFO_SOURCE}")
set(NO_LFO_FILE "${OUTPUT_DIR}/synth-engine-no-lfo.lyra")
set(NO_LFO_WAV "${OUTPUT_DIR}/synth-engine-no-lfo.wav")
file(WRITE "${NO_LFO_FILE}" "${NO_LFO_SOURCE}")
execute_process(COMMAND "${LYRA_EXE}" "${NO_LFO_FILE}" "${NO_LFO_WAV}"
    RESULT_VARIABLE NO_LFO_RESULT OUTPUT_QUIET ERROR_VARIABLE ERROR_TEXT)
if(NOT NO_LFO_RESULT EQUAL 0)
    message(FATAL_ERROR "No-LFO synth WAV failed: ${ERROR_TEXT}")
endif()
file(SHA256 "${NO_LFO_WAV}" NO_LFO_HASH)
if(UNISON_HASH STREQUAL NO_LFO_HASH)
    message(FATAL_ERROR "Modulation matrix did not change rendered audio")
endif()
