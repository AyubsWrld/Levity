cmake_minimum_required(VERSION 3.24)

project(verity
    VERSION 0.1.0
    DESCRIPTION "Policy Evaluation PoC"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(nlohmann_json REQUIRED)

add_subdirectory("${CMAKE_SOURCE_DIR}/src/common")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/spdx_parser")

add_executable(${CMAKE_PROJECT_NAME}
	"${CMAKE_SOURCE_DIR}/src/main.cpp")

target_include_directories(${CMAKE_PROJECT_NAME} # PUBLIC target depedencies are acting strangely ... 
	PRIVATE
	"${CMAKE_SOURCE_DIR}/include/spdx_parser"
	"${CMAKE_SOURCE_DIR}/include/common" 
)
target_link_libraries(${CMAKE_PROJECT_NAME}
	PRIVATE
	nlohmann_json::nlohmann_json
	tsparser
	common)

