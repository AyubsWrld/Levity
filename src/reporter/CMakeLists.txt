add_library(tsreporter)

target_sources(tsreporter
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/reporter.cpp")

find_package(nlohmann_json REQUIRED)


target_include_directories(tsreporter
  PUBLIC
  "${CMAKE_SOURCE_DIR}/include/reporter"
  "${CMAKE_SOURCE_DIR}/include/predicate"
  "${CMAKE_SOURCE_DIR}/include/parser")

target_link_libraries(tsreporter
    PUBLIC
    nlohmann_json::nlohmann_json)
