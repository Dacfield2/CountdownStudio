cmake_minimum_required(VERSION 3.28)

project(CountdownStudio VERSION 0.2.1 LANGUAGES CXX)

# Release builds derive their package/UI version from the vX.Y.Z tag.
set(COUNTDOWNSTUDIO_VERSION "${PROJECT_VERSION}")
if(DEFINED ENV{COUNTDOWNSTUDIO_RELEASE_TAG})
    set(_countdownstudio_release_tag "$ENV{COUNTDOWNSTUDIO_RELEASE_TAG}")
    if(_countdownstudio_release_tag MATCHES "^v([0-9]+\\.[0-9]+\\.[0-9]+)$")
        set(COUNTDOWNSTUDIO_VERSION "${CMAKE_MATCH_1}")
    endif()
endif()

include(CTest)

option(COUNTDOWNSTUDIO_BUILD_OBS_PLUGIN "Build the OBS Studio module (requires an OBS development package)" OFF)
set(COUNTDOWNSTUDIO_OBS_PLUGIN_LAYOUT "modern" CACHE STRING
    "OBS plugin install layout: legacy (OBS 32) or modern (OBS 33+)")
set_property(CACHE COUNTDOWNSTUDIO_OBS_PLUGIN_LAYOUT PROPERTY STRINGS legacy modern)

if(NOT COUNTDOWNSTUDIO_OBS_PLUGIN_LAYOUT STREQUAL "legacy"
   AND NOT COUNTDOWNSTUDIO_OBS_PLUGIN_LAYOUT STREQUAL "modern")
    message(FATAL_ERROR "COUNTDOWNSTUDIO_OBS_PLUGIN_LAYOUT must be 'legacy' or 'modern'.")
endif()

add_library(countdownstudio_core STATIC
    src/core/countdown.cpp
)
add_library(CountdownStudio::Core ALIAS countdownstudio_core)

target_include_directories(countdownstudio_core
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_compile_features(countdownstudio_core PUBLIC cxx_std_20)
set_target_properties(countdownstudio_core PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
    target_compile_options(countdownstudio_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(countdownstudio_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(COUNTDOWNSTUDIO_BUILD_OBS_PLUGIN)
    find_package(libobs REQUIRED)
    find_package(obs-frontend-api REQUIRED)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

    add_library(countdownstudio MODULE
        src/obs/plugin-main.cpp
        src/obs/countdown-source.cpp
        src/obs/countdown-dock.cpp
        src/obs/countdown-toolbar.cpp
    )
    target_link_libraries(countdownstudio PRIVATE CountdownStudio::Core OBS::libobs OBS::obs-frontend-api Qt6::Core Qt6::Widgets)
    target_compile_features(countdownstudio PRIVATE cxx_std_20)
    target_compile_definitions(countdownstudio PRIVATE "COUNTDOWNSTUDIO_VERSION=\"${COUNTDOWNSTUDIO_VERSION}\"")
    set_target_properties(countdownstudio PROPERTIES
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "CountdownStudio"
        PREFIX ""
    )

    if(APPLE)
        set_target_properties(countdownstudio PROPERTIES
            BUNDLE TRUE
            BUNDLE_EXTENSION "plugin"
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/Info.plist.in"
        )
        install(TARGETS countdownstudio BUNDLE DESTINATION ".")
        install(DIRECTORY data/ DESTINATION "CountdownStudio.plugin/Contents/Resources")
    elseif(COUNTDOWNSTUDIO_OBS_PLUGIN_LAYOUT STREQUAL "legacy")
        install(TARGETS countdownstudio RUNTIME DESTINATION "bin/64bit" LIBRARY DESTINATION "bin/64bit")
        install(DIRECTORY data/ DESTINATION "data")
    else()
        install(TARGETS countdownstudio RUNTIME DESTINATION "." LIBRARY DESTINATION ".")
        install(DIRECTORY data/ DESTINATION "data")
    endif()

    if(UNIX AND NOT APPLE)
        install(PROGRAMS scripts/install-linux.sh DESTINATION ".")
    endif()

    if(APPLE)
        set(COUNTDOWNSTUDIO_SOURCE_DESTINATION "CountdownStudio.plugin/Contents/Resources/source")
    else()
        set(COUNTDOWNSTUDIO_SOURCE_DESTINATION "source")
    endif()
    install(DIRECTORY src include config tests cmake data scripts
        DESTINATION "${COUNTDOWNSTUDIO_SOURCE_DESTINATION}")
    install(FILES CMakeLists.txt CMakePresets.json README.md CONTRIBUTING.md LICENSE
        DESTINATION "${COUNTDOWNSTUDIO_SOURCE_DESTINATION}")
endif()

set(CPACK_PACKAGE_NAME "CountdownStudio")
set(CPACK_PACKAGE_VENDOR "Dacfield2")
set(CPACK_PACKAGE_VERSION "${COUNTDOWNSTUDIO_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "CountdownStudio-${COUNTDOWNSTUDIO_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${COUNTDOWNSTUDIO_OBS_PLUGIN_LAYOUT}")
if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_INSTALL_ROOT "$COMMONAPPDATA/obs-studio/plugins")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "CountdownStudio")
    set(CPACK_NSIS_PACKAGE_NAME "Countdown Studio OBS Plugin")
    set(CPACK_NSIS_DISPLAY_NAME "Countdown Studio OBS Plugin")
    # Upgrade in place: an older installer may have a stale or missing uninstaller.
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL OFF)
else()
    set(CPACK_GENERATOR "ZIP")
endif()
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)

if(BUILD_TESTING)
    add_executable(countdownstudio_tests tests/test_countdown.cpp)
    target_link_libraries(countdownstudio_tests PRIVATE CountdownStudio::Core)
    target_compile_features(countdownstudio_tests PRIVATE cxx_std_20)
    set_target_properties(countdownstudio_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME countdownstudio_core COMMAND countdownstudio_tests)
endif()
