cmake_minimum_required(VERSION 3.22)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version for third-party dependencies")

project(CableJam VERSION 1.0.0 LANGUAGES C CXX RC)

option(CABLE_JAM_BUILD_TESTS "Build unit tests" ON)

include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:C>:/utf-8>"
        "$<$<COMPILE_LANGUAGE:CXX>:/utf-8>"
        "$<$<COMPILE_LANGUAGE:CXX>:/EHsc>"
    )
    add_compile_options("$<$<CONFIG:Release>:/Os>")
endif()

FetchContent_Declare(
    cjson_src
    URL https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.19.zip
)
set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CJSON_OVERRIDE_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(CJSON_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cjson_src)
set(BUILD_TESTING ON CACHE BOOL "" FORCE)

add_library(cjson_static STATIC ${cjson_src_SOURCE_DIR}/cJSON.c)
target_include_directories(cjson_static PUBLIC ${cjson_src_SOURCE_DIR})
target_compile_definitions(cjson_static PRIVATE CJSON_HIDE_SYMBOLS)

set(CABLE_CORE_SOURCES
    src/core/Tween.cpp
    src/core/WinFile.cpp
    src/game/GameState.cpp
    src/game/LevelGenerator.cpp
    src/game/ProgressStore.cpp
)

add_library(cable_core STATIC ${CABLE_CORE_SOURCES})
target_include_directories(cable_core PUBLIC src)
target_link_libraries(cable_core PUBLIC cjson_static)
target_compile_definitions(cable_core PUBLIC
    WINVER=0x0602
    _WIN32_WINNT=0x0602
    UNICODE
    _UNICODE
    NOMINMAX
)

set(CABLE_APP_SOURCES
    src/app/App.cpp
    src/app/Dpi.cpp
    src/app/Window.cpp
    src/audio/AudioEngine.cpp
    src/graphics/D2DContext.cpp
    src/ui/Scenes.cpp
    src/ui/Overlays.cpp
)

add_library(cable_app STATIC ${CABLE_APP_SOURCES})
target_include_directories(cable_app PUBLIC src)
target_link_libraries(cable_app PUBLIC cable_core)
target_compile_definitions(cable_app PUBLIC
    WINVER=0x0602
    _WIN32_WINNT=0x0602
    UNICODE
    _UNICODE
    NOMINMAX
)
target_link_libraries(cable_app PUBLIC
    d2d1 dwrite windowscodecs xaudio2_8 ole32 uuid comctl32
    gdi32 user32
)

add_executable(cable_jam WIN32
    src/app/WinMain.cpp
    src/resources/resources.rc
)
target_link_libraries(cable_jam PRIVATE cable_app)

add_executable(scene_viewer
    tests/scene_viewer/SceneViewer.cpp
    src/resources/resources.rc
)
target_link_libraries(scene_viewer PRIVATE cable_app)

if(CABLE_JAM_BUILD_TESTS)
    FetchContent_Declare(
        doctest_src
        URL https://github.com/doctest/doctest/archive/refs/tags/v2.5.2.zip
    )
    FetchContent_MakeAvailable(doctest_src)

    add_executable(unit_tests
        tests/unit/TestMain.cpp
        tests/unit/GameLogicTests.cpp
    )
    target_link_libraries(unit_tests PRIVATE cable_core doctest::doctest)
    target_compile_definitions(unit_tests PRIVATE DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS)

    enable_testing()
    add_test(NAME unit_tests COMMAND unit_tests)
    add_test(NAME ui_scene_main COMMAND scene_viewer --scene main --screenshot ${CMAKE_BINARY_DIR}/ui-main.jpg --quality 75)
    add_test(NAME ui_scene_levels COMMAND scene_viewer --scene levels --mock progress --screenshot ${CMAKE_BINARY_DIR}/ui-levels.jpg --quality 75)
    add_test(NAME ui_scene_game_low COMMAND scene_viewer --scene game --screenshot ${CMAKE_BINARY_DIR}/ui-game-low.jpg --quality 75)
    add_test(NAME ui_scene_game_high COMMAND scene_viewer --scene game-high --screenshot ${CMAKE_BINARY_DIR}/ui-game-high.jpg --quality 75)
    add_test(NAME ui_overlay_complete COMMAND scene_viewer --scene game --overlay complete --screenshot ${CMAKE_BINARY_DIR}/ui-complete.jpg --quality 75)
    add_test(NAME ui_overlay_help COMMAND scene_viewer --scene main --overlay help --screenshot ${CMAKE_BINARY_DIR}/ui-help.jpg --quality 75)
    add_test(NAME ui_overlay_about COMMAND scene_viewer --scene main --overlay about --screenshot ${CMAKE_BINARY_DIR}/ui-about.jpg --quality 75)
    add_test(NAME ui_overlay_confirm_exit COMMAND scene_viewer --scene main --overlay confirm-exit --screenshot ${CMAKE_BINARY_DIR}/ui-confirm-exit.jpg --quality 75)
    add_test(NAME ui_click_main_start COMMAND scene_viewer --scene main --click 640 255 --screenshot ${CMAKE_BINARY_DIR}/ui-click-main-start.jpg --quality 75)
    add_test(NAME ui_animation_pull COMMAND scene_viewer --scene game --click 350 155 --frames-after-click 30 --screenshot ${CMAKE_BINARY_DIR}/ui-animation-pull.jpg --quality 75)
endif()
