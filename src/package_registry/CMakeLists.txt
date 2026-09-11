add_library(tspkgregistry
  "${CMAKE_CURRENT_SOURCE_DIR}/package_registry.cpp")

find_package(nlohmann_json REQUIRED)

target_include_directories(tspkgregistry
  PUBLIC
  "${CMAKE_SOURCE_DIR}/include/parser"
  "${CMAKE_SOURCE_DIR}/include/package_registry"
  "${CMAKE_SOURCE_DIR}/include/predicate"
  "${CMAKE_SOURCE_DIR}/include/utils")


target_link_libraries(tspkgregistry
  PUBLIC
  tsparser
  tspredicate
  PRIVATE
  nlohmann_json::nlohmann_json)
