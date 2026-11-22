cmake_minimum_required(VERSION 3.22)

# Solo afecta a macOS: universal (Apple Silicon + Intel), macOS 11 o superior
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

project(MonoCheck VERSION 1.7.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4)
FetchContent_MakeAvailable(JUCE)

# Copiar el plugin a la carpeta de plugins tras compilar (en Windows requiere admin, por eso OFF)
if(APPLE)
    option(COPY_AFTER_BUILD "Copiar el plugin a la carpeta de plugins" ON)
else()
    option(COPY_AFTER_BUILD "Copiar el plugin a la carpeta de plugins" OFF)
endif()

# Formatos: VST3 en todos los sistemas; AU (Audio Components) solo en macOS
if(APPLE)
    set(PLUGIN_FORMATS VST3 AU)
else()
    set(PLUGIN_FORMATS VST3)
endif()

juce_add_plugin(MonoCheck
    COMPANY_NAME "TxuSo"
    PLUGIN_MANUFACTURER_CODE Mest
    PLUGIN_CODE Mchk
    FORMATS ${PLUGIN_FORMATS}
    VST3_CATEGORIES Fx Analyzer
    PRODUCT_NAME "MonoCheck"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER_BUILD})

target_sources(MonoCheck PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(MonoCheck PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MonoCheck PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_basics
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
