add_library(edge_common INTERFACE)

target_include_directories(edge_common
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if(EDGE_BUILD_TESTS)
    edge_add_test_executable(edge_common_tests
        LABEL unit
        SOURCES
            test/result_test.cpp
    )
    target_link_libraries(edge_common_tests PRIVATE edge_common)
endif()

add_subdirectory(schemas)

if(NOT TARGET edge_platform)
    add_subdirectory(libs/platform)
endif()

if(NOT TARGET edge_ipc)
    add_subdirectory(libs/ipc)
endif()
