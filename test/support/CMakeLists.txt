add_library(edge_test_support INTERFACE)

target_include_directories(edge_test_support
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
