cmake_minimum_required(VERSION 3.22)
project(BaakProductionMaster VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
option(BAAK_CORE_ONLY "Build only dependency-free DSP tests" OFF)
enable_testing()
add_executable(baak_dsp_tests tests/dsp_tests.cpp)
target_include_directories(baak_dsp_tests PRIVATE Source)
add_test(NAME dsp COMMAND baak_dsp_tests)
if(BAAK_CORE_ONLY)
  return()
endif()
set(JUCE_DIR "" CACHE PATH "Path to JUCE checkout")
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG d60b7a78030661574a615b586d9325ba7a7fa7dd)
  FetchContent_MakeAvailable(JUCE)
else()
  add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
endif()
set(BAAK_FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND BAAK_FORMATS AU)
endif()
juce_add_plugin(BaakMaster
  COMPANY_NAME "BaakProduction"
  BUNDLE_ID "com.baakproduction.master"
  PLUGIN_MANUFACTURER_CODE Baak
  PLUGIN_CODE Bpm1
  PRODUCT_NAME "BaakProduction Master"
  FORMATS ${BAAK_FORMATS}
  IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Fx Mastering)
target_sources(BaakMaster PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(BaakMaster PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BaakMaster PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
option(BAAK_VALIDATION "Build native processor and UI smoke-test harness" ON)
if(BAAK_VALIDATION)
  juce_add_console_app(baak_native_tests PRODUCT_NAME "Baak native validation")
  target_sources(baak_native_tests PRIVATE tests/native_tests.cpp)
  target_include_directories(baak_native_tests PRIVATE Source)
  target_compile_definitions(baak_native_tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1)
  target_include_directories(baak_native_tests PRIVATE $<TARGET_PROPERTY:juce::juce_audio_utils,INTERFACE_INCLUDE_DIRECTORIES>)
  target_link_libraries(baak_native_tests PRIVATE BaakMaster)
endif()
