cmake_minimum_required(VERSION 3.22)
project(WRECKAGE VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
endif()
if(MSVC)
    # Static CRT so the .vst3 needs no VC++ redistributable on his machine.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Local JUCE checkout if present (fast dev), otherwise fetch the pinned tag (CI).
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt)
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Wreckage
    COMPANY_NAME "Austin Radliff"
    PRODUCT_NAME "WRECKAGE"
    BUNDLE_ID "com.austinradliff.wreckage"
    PLUGIN_MANUFACTURER_CODE Arad
    PLUGIN_CODE Wrek
    FORMATS VST3 AU Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_WEBVIEW2 TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice)

file(GLOB UI_FILES CONFIGURE_DEPENDS ui/*.html ui/*.css ui/*.js ui/assets/*.ttf ui/assets/*.webp assets/plates/*.png)
juce_add_binary_data(WreckageData SOURCES ${UI_FILES} presets/factory.json)

target_sources(Wreckage PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets.cpp)
if(APPLE)
    target_sources(Wreckage PRIVATE Source/FocusYield_mac.mm)
elseif(WIN32)
    target_sources(Wreckage PRIVATE Source/FocusYield_win.cpp)
endif()

target_compile_definitions(Wreckage PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Wreckage
    PRIVATE WreckageData
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_gui_extra
    PUBLIC  juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

# Offline check: renders every factory preset and fails on silence, NaN or clipping blowups.
juce_add_console_app(WreckageRender PRODUCT_NAME "wreckage-render")
target_sources(WreckageRender PRIVATE tools/render.cpp Source/Presets.cpp Source/PluginProcessor.cpp)
target_include_directories(WreckageRender PRIVATE Source)
target_compile_definitions(WreckageRender PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 WRECKAGE_HEADLESS=1)
target_link_libraries(WreckageRender PRIVATE WreckageData juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags)

# Host-behaviour check: odd sample rates and blocks larger than prepareToPlay promised.
juce_add_console_app(WreckageHostSim PRODUCT_NAME "wreckage-hostsim")
target_sources(WreckageHostSim PRIVATE tools/hostsim.cpp Source/Presets.cpp Source/PluginProcessor.cpp)
target_include_directories(WreckageHostSim PRIVATE Source)
target_compile_definitions(WreckageHostSim PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 WRECKAGE_HEADLESS=1)
target_link_libraries(WreckageHostSim PRIVATE WreckageData juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)

if(APPLE) # dev tool: loads the installed AU/VST3
juce_add_console_app(WreckagePlugHost PRODUCT_NAME "wreckage-plughost")
target_sources(WreckagePlugHost PRIVATE tools/plughost.cpp)
target_compile_definitions(WreckagePlugHost PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_AU=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(WreckagePlugHost PRIVATE juce::juce_audio_processors juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags)
endif()
