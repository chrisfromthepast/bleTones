cmake_minimum_required(VERSION 3.22)
project(bleTones_helper VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# The helper is a lightweight native executable: no JUCE dependency.
# It uses platform BLE APIs and a self-contained UDP OSC sender.
# ---------------------------------------------------------------------------

if(APPLE)
    # macOS – CoreBluetooth (Objective-C++)
    add_executable(bleTones_helper MACOSX_BUNDLE
        Source/main.mm
    )

    set_target_properties(bleTones_helper PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST          "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
        MACOSX_BUNDLE_BUNDLE_NAME         "bleTones Helper"
        MACOSX_BUNDLE_BUNDLE_IDENTIFIER   "com.chrisfromthepast.bletones-helper"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_BUNDLE_VERSION      "1"
    )

    target_link_libraries(bleTones_helper PRIVATE
        "-framework CoreBluetooth"
        "-framework Foundation"
    )

elseif(WIN32)
    # Windows – WinRT BLE (Windows.Devices.Bluetooth)
    add_executable(bleTones_helper
        Source/main_win.cpp
    )

    target_link_libraries(bleTones_helper PRIVATE
        WindowsApp
    )

    # Require Windows 10 for WinRT BLE APIs
    target_compile_definitions(bleTones_helper PRIVATE
        WINVER=0x0A00
        _WIN32_WINNT=0x0A00
    )

else()
    message(STATUS "bleTones helper: Linux/BlueZ support not yet implemented")
endif()

target_include_directories(bleTones_helper PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
