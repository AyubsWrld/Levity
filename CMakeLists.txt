cmake_minimum_required(VERSION 3.24)

project(align
    VERSION 0.1.0
    DESCRIPTION "Policy Evaluation PoC"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory("${CMAKE_SOURCE_DIR}/src/spdx_parser")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/predicates")

find_package(nlohmann_json REQUIRED)

add_executable(${CMAKE_PROJECT_NAME}
	"${CMAKE_SOURCE_DIR}/src/main.cpp")

target_link_libraries(${CMAKE_PROJECT_NAME}
	PRIVATE
	nlohmann_json::nlohmann_json
	tsparser
	tspredicates)

