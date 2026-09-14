cmake_minimum_required(VERSION 3.24)

project(rbuff
        VERSION 0.1.0
        DESCRIPTION "Policy Evaluation PoC"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(nlohmann_json REQUIRED)

add_subdirectory("${CMAKE_SOURCE_DIR}/src/spdx_parser")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/predicate")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/executor")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/package_registry")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/locator")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/linkage")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/reporter")

add_executable(${CMAKE_PROJECT_NAME}
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PUBLIC
        "include/utils/"
        "include/rules/"
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        tspredicate
        tsparser
        tsreporter
        tsexecutor
        tslinkage
)
