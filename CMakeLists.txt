cmake_minimum_required(VERSION 3.24)

project(align
        VERSION 0.1.0
        LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory("${CMAKE_SOURCE_DIR}/src/parser")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/predicate")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/package_registry")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/executor")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/notifier")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/locator")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/linkage")

add_executable(${CMAKE_PROJECT_NAME} src/main.cpp)

target_include_directories(${CMAKE_PROJECT_NAME}
        PRIVATE
        "${CMAKE_SOURCE_DIR}/include/parser"
        "${CMAKE_SOURCE_DIR}/include/rules"
)

target_link_libraries(${CMAKE_PROJECT_NAME}
        PRIVATE
        tsparser
        tspkgregistry
        tslocator
        tsexecutor
        tslinkage
        tsnotifier
)

enable_testing()
add_subdirectory("${CMAKE_SOURCE_DIR}/tests")
