add_library(tslocator
  "${CMAKE_CURRENT_SOURCE_DIR}/locator.cpp")

target_include_directories(tslocator
  PUBLIC
  "${CMAKE_SOURCE_DIR}/include/locator"
  "${CMAKE_SOURCE_DIR}/include/predicate"
  "${CMAKE_SOURCE_DIR}/include/package_registry")

target_link_libraries(tslocator
  PUBLIC
  tspredicate
  tspkgregistry)



