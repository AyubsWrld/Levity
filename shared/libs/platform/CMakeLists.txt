find_package(Threads REQUIRED)

add_library(edge_platform INTERFACE)
target_include_directories(edge_platform INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(edge_platform INTERFACE Threads::Threads)

if(EDGE_BUILD_TESTS)
    edge_add_test_executable(edge_platform_tests
        LABEL unit
        SOURCES
            test/logger_test.cpp
    )
    target_link_libraries(edge_platform_tests PRIVATE edge_platform)
endif()
