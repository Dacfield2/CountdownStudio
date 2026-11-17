cmake_minimum_required(VERSION 3.28)

project(CountdownStudio VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

option(COUNTDOWNSTUDIO_BUILD_OBS_PLUGIN "Build the OBS Studio module (requires an OBS development package)" OFF)

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

    add_library(countdownstudio MODULE
        src/obs/plugin-main.cpp
    )
    target_link_libraries(countdownstudio PRIVATE CountdownStudio::Core OBS::libobs)
    target_compile_features(countdownstudio PRIVATE cxx_std_20)
    set_target_properties(countdownstudio PROPERTIES
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "CountdownStudio"
        PREFIX ""
    )

    install(TARGETS countdownstudio
        LIBRARY DESTINATION "obs-plugins"
        RUNTIME DESTINATION "obs-plugins"
    )
    install(DIRECTORY data/locale/
        DESTINATION "data/obs-plugins/CountdownStudio/locale"
    )
endif()

if(BUILD_TESTING)
    add_executable(countdownstudio_tests tests/test_countdown.cpp)
    target_link_libraries(countdownstudio_tests PRIVATE CountdownStudio::Core)
    target_compile_features(countdownstudio_tests PRIVATE cxx_std_20)
    set_target_properties(countdownstudio_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME countdownstudio_core COMMAND countdownstudio_tests)
endif()
