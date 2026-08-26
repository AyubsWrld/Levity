# Shared configuration applied to every project-owned C++ target.
#
# Third-party targets are deliberately untouched: warnings, sanitizers and clang-tidy are scoped to
# code this repository owns.

set(EDGE_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2)

# Sanitizer flags are computed once and applied to compile *and* link steps of project targets.
set(EDGE_SANITIZER_FLAGS "")

if(EDGE_ENABLE_ASAN AND EDGE_ENABLE_TSAN)
    message(FATAL_ERROR "EDGE_ENABLE_ASAN and EDGE_ENABLE_TSAN are mutually incompatible.")
endif()

if(EDGE_ENABLE_ASAN)
    list(APPEND EDGE_SANITIZER_FLAGS -fsanitize=address -fno-omit-frame-pointer)
endif()

if(EDGE_ENABLE_UBSAN)
    list(APPEND EDGE_SANITIZER_FLAGS -fsanitize=undefined -fno-omit-frame-pointer)
endif()

if(EDGE_ENABLE_TSAN)
    list(APPEND EDGE_SANITIZER_FLAGS -fsanitize=thread -fno-omit-frame-pointer)
endif()

# Resolve clang-tidy once so that per-target configuration stays cheap.
set(EDGE_CLANG_TIDY_COMMAND "")

if(NOT SKIP_CLANG_TIDY)
    find_program(EDGE_CLANG_TIDY_EXECUTABLE NAMES clang-tidy)

    if(EDGE_CLANG_TIDY_EXECUTABLE)
        # Mirror the root CMakeLists.txt global CMAKE_CXX_CLANG_TIDY flags exactly, so that
        # per-target CXX_CLANG_TIDY overrides applied below do not silently weaken enforcement
        # (--warnings-as-errors=* makes clang-tidy findings fail the build; --extra-arg=-w
        # suppresses plain compiler warnings so only clang-tidy diagnostics are reported).
        set(EDGE_CLANG_TIDY_COMMAND "${EDGE_CLANG_TIDY_EXECUTABLE};--warnings-as-errors=*;--extra-arg=-w")
        message(STATUS "clang-tidy enabled: ${EDGE_CLANG_TIDY_EXECUTABLE}")
    else()
        message(WARNING "SKIP_CLANG_TIDY=OFF but clang-tidy was not found; continuing without it.")
    endif()
endif()

# Apply the repository's C++23 standard, warning set, sanitizer configuration and (optionally)
# clang-tidy to a project-owned target.
#
# edge_configure_target(<target> [NO_CLANG_TIDY])
function(edge_configure_target target)
    cmake_parse_arguments(EDGE_ARG "NO_CLANG_TIDY" "" "" ${ARGN})

    target_compile_features(${target} PUBLIC cxx_std_20)
    target_compile_options(${target} PRIVATE ${EDGE_WARNING_FLAGS})

    if(EDGE_WARNINGS_AS_ERRORS)
        target_compile_options(${target} PRIVATE -Werror)
    endif()

    if(EDGE_SANITIZER_FLAGS)
        target_compile_options(${target} PRIVATE ${EDGE_SANITIZER_FLAGS})
        target_link_options(${target} PRIVATE ${EDGE_SANITIZER_FLAGS})
    endif()

    if(EDGE_CLANG_TIDY_COMMAND AND NOT EDGE_ARG_NO_CLANG_TIDY)
        set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${EDGE_CLANG_TIDY_COMMAND}")
    endif()
endfunction()

# Register a GoogleTest executable with CTest.
#
# edge_add_test_executable(<target> LABEL <ctest-label> SOURCES <src>...)
function(edge_add_test_executable target)
    cmake_parse_arguments(EDGE_TEST "" "LABEL" "SOURCES" ${ARGN})

    if(NOT EDGE_TEST_LABEL)
        set(EDGE_TEST_LABEL "unit")
    endif()

    add_executable(${target} ${EDGE_TEST_SOURCES})
    edge_configure_target(${target})
    target_link_libraries(${target} PRIVATE GTest::gtest GTest::gmock GTest::gtest_main)

    gtest_discover_tests(${target}
        PROPERTIES LABELS "${EDGE_TEST_LABEL}"
        DISCOVERY_TIMEOUT 60)
endfunction()
