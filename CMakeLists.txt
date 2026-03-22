cmake_minimum_required(VERSION 3.22)
project(bleTones VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Fetch JUCE (pinned to a stable release tag)
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# bleTones VST3 / AU / Standalone plugin
# ---------------------------------------------------------------------------
juce_add_plugin(bleTones
    COMPANY_NAME                "chrisfromthepast"
    PLUGIN_MANUFACTURER_CODE    Cftp
    PLUGIN_CODE                 BleT
    FORMATS                     AU VST3 Standalone
    PRODUCT_NAME                "bleTones"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
)

target_sources(bleTones PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(bleTones PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(bleTones
    PRIVATE
        juce::juce_audio_utils
        juce::juce_osc
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

juce_generate_juce_header(bleTones)

# ---------------------------------------------------------------------------
# BLE Helper companion process
# ---------------------------------------------------------------------------
add_subdirectory(helper)

# ---------------------------------------------------------------------------
# macOS Standalone: embed bleTones_helper.app inside the .app bundle and
# ensure the helper is built before we try to copy it.
# ---------------------------------------------------------------------------
if(APPLE)
    add_dependencies(bleTones_Standalone bleTones_helper)

    add_custom_command(
        TARGET bleTones_Standalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:bleTones_Standalone>/Contents/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:bleTones_helper>"
            "$<TARGET_BUNDLE_DIR:bleTones_Standalone>/Contents/Resources/bleTones_helper.app"
        COMMENT "Embedding bleTones_helper.app into Standalone bundle Resources"
    )
endif()
