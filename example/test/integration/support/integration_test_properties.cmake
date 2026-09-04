# Companion CTest include script for `integration_tests` (see test/integration/CMakeLists.txt for
# why this file exists).
#
# `gtest_discover_tests()` uses POST_BUILD discovery: real test names are not known at `cmake`
# configure time (they are only known after the binary is built and introspected via
# `--gtest_list_tests`), so `edge_add_test_executable()` cannot set a per-test TIMEOUT itself - its
# `PROPERTIES` argument only ever carries `LABELS`. This is the CMake-documented mechanism for
# attaching additional properties to tests that are not known yet at configure time: a companion
# CTest TEST_INCLUDE_FILES script, keyed off the `<target>_TESTS` variable that
# `gtest_discover_tests()` populates in this exact script context for exactly this purpose (see the
# GoogleTest.cmake module's own "TEST_LIST"/TEST_INCLUDE_FILES documentation).
if(DEFINED integration_tests_TESTS)
    set_tests_properties(${integration_tests_TESTS} PROPERTIES TIMEOUT 120)
endif()
