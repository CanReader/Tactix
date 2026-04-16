# Copyright Sleak Software. All Rights Reserved.
#
# Standalone CMake harness that compiles TactixCore directly — with NO Unreal
# Engine dependency — and links the sources against GoogleTest to run the unit
# suite locally or on CI. This satisfies the Phase 1 rule that TactixCore must
# compile and pass tests independently of UE.
#
# Typical build (from Plugins/Tactix/Tests):
#     cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
#     cmake --build build -j
#     ctest --test-dir build --output-on-failure
#
# If GTest is not installed system-wide, the harness falls back to FetchContent
# and grabs a pinned release of googletest.

cmake_minimum_required(VERSION 3.20)
project(TactixTests LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Project-wide compile settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD           20)
set(CMAKE_CXX_STANDARD_REQUIRED  ON)
set(CMAKE_CXX_EXTENSIONS         OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Matches the CLAUDE.md "No warnings" requirement — treat-as-warnings on Clang/GCC.
if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wno-unused-parameter)
endif()

# TactixCore headers check TACTIX_DEBUG for the assert macro; enable in Debug.
add_compile_definitions($<$<CONFIG:Debug>:TACTIX_DEBUG=1>)

# ---------------------------------------------------------------------------
# Locate the plugin and TactixCore source tree
# ---------------------------------------------------------------------------
set(TACTIX_PLUGIN_ROOT     "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(TACTIX_CORE_ROOT       "${TACTIX_PLUGIN_ROOT}/Source/TactixCore")
set(TACTIX_CORE_PUBLIC     "${TACTIX_CORE_ROOT}/Public")
set(TACTIX_CORE_PRIVATE    "${TACTIX_CORE_ROOT}/Private")
set(TACTIX_SYSTEMS_ROOT    "${TACTIX_PLUGIN_ROOT}/Source/TactixSystems")
set(TACTIX_SYSTEMS_PUBLIC  "${TACTIX_SYSTEMS_ROOT}/Public")
set(TACTIX_SYSTEMS_PRIVATE "${TACTIX_SYSTEMS_ROOT}/Private")

# TactixCore compile units (excluding the UE-only module entry point).
set(TACTIX_CORE_SOURCES
    "${TACTIX_CORE_PRIVATE}/Foundation/TactixCurve.cpp"
    "${TACTIX_CORE_PRIVATE}/Agent/FTactixAgentScheduler.cpp"
)

set(TACTIX_SYSTEMS_SOURCES
    "${TACTIX_SYSTEMS_PRIVATE}/GOAP/FTactixGOAPPlanner.cpp"
    "${TACTIX_SYSTEMS_PRIVATE}/HTN/FTactixHTNPlanner.cpp"
)

# ---------------------------------------------------------------------------
# Static libraries: TactixCore and TactixSystems compiled without UE
# ---------------------------------------------------------------------------
add_library(TactixCoreStandalone STATIC ${TACTIX_CORE_SOURCES})

target_include_directories(TactixCoreStandalone PUBLIC
    "${TACTIX_CORE_PUBLIC}"
    "${TACTIX_CORE_PRIVATE}"
)

set_target_properties(TactixCoreStandalone PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

add_library(TactixSystemsStandalone STATIC ${TACTIX_SYSTEMS_SOURCES})

target_include_directories(TactixSystemsStandalone PUBLIC
    "${TACTIX_SYSTEMS_PUBLIC}"
)

target_link_libraries(TactixSystemsStandalone PUBLIC TactixCoreStandalone)

set_target_properties(TactixSystemsStandalone PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# ---------------------------------------------------------------------------
# GoogleTest — prefer installed, fall back to FetchContent
# ---------------------------------------------------------------------------
find_package(GTest CONFIG QUIET)

if (NOT GTest_FOUND)
    message(STATUS "GTest not found via find_package — falling back to FetchContent.")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    # Prevent gtest from overriding our compiler/linker options on Windows.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# ---------------------------------------------------------------------------
# Test executable
# ---------------------------------------------------------------------------
set(TACTIX_TEST_SOURCES
    "CoreTests/TestHandle.cpp"
    "CoreTests/TestPool.cpp"
    "CoreTests/TestArena.cpp"
    "CoreTests/TestCurve.cpp"
    "CoreTests/TestMath.cpp"
    "CoreTests/TestGrid.cpp"
    "CoreTests/TestEventBus.cpp"
    "CoreTests/TestBlackboard.cpp"
    "CoreTests/TestPerceptionMemory.cpp"
    "CoreTests/TestScheduler.cpp"
    "SystemsTests/TestUtility.cpp"
    "SystemsTests/TestGOAP.cpp"
    "SystemsTests/TestHTN.cpp"
    "SystemsTests/TestCover.cpp"
    "SystemsTests/TestInfluence.cpp"
    "SystemsTests/TestSquad.cpp"
)

add_executable(TactixTests ${TACTIX_TEST_SOURCES})

target_link_libraries(TactixTests PRIVATE
    TactixCoreStandalone
    TactixSystemsStandalone
    GTest::gtest
    GTest::gtest_main
)

# ---------------------------------------------------------------------------
# CTest registration
# ---------------------------------------------------------------------------
enable_testing()
include(GoogleTest)
gtest_discover_tests(TactixTests)
