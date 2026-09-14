add_library(tsexecutor
  "${CMAKE_CURRENT_SOURCE_DIR}/predicate_executor.cpp")

target_include_directories(tsexecutor
  PUBLIC
  "${CMAKE_SOURCE_DIR}/include/executor"
  "${CMAKE_SOURCE_DIR}/include/locator"
  "${CMAKE_SOURCE_DIR}/include/package_registry")



target_link_libraries(tsexecutor
  PUBLIC
  tspredicate
  tslocator)
