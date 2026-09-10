add_library(tsparser
  "${CMAKE_CURRENT_SOURCE_DIR}/parser.cpp")


find_package(nlohmann_json REQUIRED)

target_include_directories(tsparser
  PUBLIC
  "${CMAKE_SOURCE_DIR}/include/parser")

target_link_libraries(tsparser
  PUBLIC
  nlohmann_json::nlohmann_json)
