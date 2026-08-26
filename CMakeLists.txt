cmake_minimum_required(VERSION 3.24)

project(edge_isr_platform
    VERSION 0.1.0
    DESCRIPTION "Modular Edge ISR API Platform - Tier 1 Slew-to-Cue MVP"
    LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ── Conan auto-install ────────────────────────────────────────────────────────
# Runs conan install at configure time so that adding a new dependency to
# conanfile.py does not require a manual conan install step. This is fast
# inside the SDK container since all packages are already pre-built there.
find_program(CONAN_EXECUTABLE conan)
if(CONAN_EXECUTABLE)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            "${CMAKE_SOURCE_DIR}/conanfile.py")
    message(STATUS "Running conan install (build_type=${CMAKE_BUILD_TYPE})...")
    # conanfile.py sets self.folders.generators = "." and we pass
    # --output-folder "${CMAKE_BINARY_DIR}" below, so generators land directly in
    # the active build directory, whatever it is named. This is what lets any
    # build directory (including CLion's default "cmake-build-debug") work
    # without extra flags such as a manual conan install or toolchain file.
    execute_process(
            COMMAND "${CONAN_EXECUTABLE}" install "${CMAKE_SOURCE_DIR}"
            --build=missing
            --output-folder "${CMAKE_BINARY_DIR}"
            -s "build_type=${CMAKE_BUILD_TYPE}"
            -s "compiler.cppstd=${CMAKE_CXX_STANDARD}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE CONAN_RESULT
            OUTPUT_VARIABLE CONAN_OUTPUT
            ERROR_VARIABLE CONAN_OUTPUT
    )
    if(NOT CONAN_RESULT EQUAL 0)
        message(FATAL_ERROR "conan install failed:\n${CONAN_OUTPUT}")
    endif()
    set(CONAN_GENERATORS_DIR "${CMAKE_BINARY_DIR}")
    list(PREPEND CMAKE_PREFIX_PATH "${CONAN_GENERATORS_DIR}")
    list(PREPEND CMAKE_MODULE_PATH "${CONAN_GENERATORS_DIR}")
else()
    message(WARNING "conan not found — skipping auto-install")
    set(CONAN_GENERATORS_DIR "${CMAKE_BINARY_DIR}")
    list(PREPEND CMAKE_PREFIX_PATH "${CONAN_GENERATORS_DIR}")
endif()
message(STATUS "Conan setup done. (CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH})...")

# Run the three executables straight out of the build tree (CLion compound run configurations).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(SKIP_CLANG_TIDY "Disable the clang-tidy integration for fast local iteration" ON)
option(EDGE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(EDGE_BUILD_TESTS "Build unit and integration tests" ON)
option(EDGE_BUILD_BENCHMARKS "Build the IPC latency benchmark" ON)
option(EDGE_ENABLE_ASAN "Enable AddressSanitizer for project targets" OFF)
option(EDGE_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer for project targets" OFF)
option(EDGE_ENABLE_TSAN "Enable ThreadSanitizer for project targets" OFF)

if( NOT SKIP_CLANG_TIDY )
    set( CMAKE_CXX_CLANG_TIDY
            clang-tidy;
            --warnings-as-errors=*;
            --extra-arg=-w;
    )
endif()

find_package(protobuf CONFIG REQUIRED)
find_package(ZeroMQ REQUIRED)
find_package(cppzmq REQUIRED)
find_package(httplib REQUIRED)
find_package(nlohmann_json REQUIRED)

find_package(Threads REQUIRED)

if(EDGE_BUILD_TESTS)
    find_package(GTest REQUIRED)
    include(GoogleTest)
    enable_testing()
endif()

include(edge_target_helpers)
include(architecture_checks)

# Subdirectories are guarded so that the repository stays configurable while it is bootstrapped
# incrementally.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/shared/CMakeLists.txt")
    add_subdirectory(shared)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/app/dummy_monolith/CMakeLists.txt")
    add_subdirectory(app/dummy_monolith)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/services/api_gateway/CMakeLists.txt")
    add_subdirectory(services/api_gateway)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/services/gimbal_controller/CMakeLists.txt")
    add_subdirectory(services/gimbal_controller)
endif()

if(EDGE_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/integration/CMakeLists.txt")
    add_subdirectory(test/integration)
endif()

edge_verify_architecture()
