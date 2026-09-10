add_library(tspredicate)

target_sources(tspredicate
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/predicate.cpp")

target_include_directories(tspredicate
  PUBLIC
  "${CMAKE_SOURCE_DIR}/include/predicate")

