# Deterministic ELF fixtures (Part 31): a real shared library with a real
# SONAME, an executable that links it (DT_NEEDED present), and one that
# doesn't -- so dynamic-link tests don't depend on whatever happens to be
# installed on the host.
add_library(lgpl_fixture SHARED
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/lgpl_fixture_lib.cpp")
set_target_properties(lgpl_fixture PROPERTIES
  OUTPUT_NAME lgpl_fixture
  VERSION 1.0.0
  SOVERSION 1)

add_executable(dynamic_fixture
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/dynamic_fixture_main.cpp")
target_link_libraries(dynamic_fixture PRIVATE lgpl_fixture)

add_executable(unlinked_fixture
  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/unlinked_fixture_main.cpp")

set(TSTEST_FIXTURE_HEADER "${CMAKE_CURRENT_BINARY_DIR}/fixture_paths.hpp")
file(GENERATE OUTPUT "${TSTEST_FIXTURE_HEADER}" CONTENT
"#pragma once
namespace fixtures {
inline constexpr const char *kLgplFixtureLib = \"$<TARGET_FILE:lgpl_fixture>\";
inline constexpr const char *kDynamicFixtureBinary = \"$<TARGET_FILE:dynamic_fixture>\";
inline constexpr const char *kUnlinkedFixtureBinary = \"$<TARGET_FILE:unlinked_fixture>\";
} // namespace fixtures
")

add_executable(rbuff_tests
  "${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_predicate_registry.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_predicate_execution.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_dynamic_link.cpp")

# The fixtures aren't linked into rbuff_tests (they're inspected via readelf,
# not called into), so the dependency must be declared explicitly.
add_dependencies(rbuff_tests lgpl_fixture dynamic_fixture unlinked_fixture)

target_include_directories(rbuff_tests
  PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_BINARY_DIR}"
  "${CMAKE_SOURCE_DIR}/include/rules"
  "${CMAKE_SOURCE_DIR}/include/utils")

target_link_libraries(rbuff_tests
  PRIVATE
  tspredicate
  tslinkage
  tsparser)

add_test(NAME rbuff_tests COMMAND rbuff_tests)
