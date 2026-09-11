add_library(tslinkage
  "${CMAKE_CURRENT_SOURCE_DIR}/purl.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/process_runner.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/elf.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/package_resolver.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/dynamic_link.cpp")

find_package(nlohmann_json REQUIRED)

target_include_directories(tslinkage
  PUBLIC
  "${CMAKE_SOURCE_DIR}/include/linkage"
  "${CMAKE_SOURCE_DIR}/include/parser")

target_link_libraries(tslinkage
  PRIVATE
  nlohmann_json::nlohmann_json)
