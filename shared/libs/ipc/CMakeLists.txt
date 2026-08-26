add_library(edge_ipc STATIC
    src/context.cpp
    src/endpoint.cpp
    src/detail/endpoint.cpp
    src/detail/socket.cpp
    src/publisher.cpp
    src/subscriber.cpp
    src/requester.cpp
    src/replier.cpp
)

target_include_directories(edge_ipc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(edge_ipc
    PUBLIC
        edge_common
    PRIVATE
        cppzmq
        libzmq-static
)
edge_configure_target(edge_ipc)

if((EDGE_BUILD_TESTS OR EDGE_BUILD_BENCHMARKS) AND NOT TARGET edge_test_support)
    add_subdirectory(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../test/support
        ${CMAKE_BINARY_DIR}/test/support
    )
endif()

if(EDGE_BUILD_TESTS)
    edge_add_test_executable(edge_ipc_tests
        LABEL unit
        SOURCES
            test/endpoint_test.cpp
            test/pubsub_test.cpp
            test/reqrep_test.cpp
            test/max_message_size_test.cpp
    )
    target_link_libraries(edge_ipc_tests PRIVATE edge_ipc edge_test_support)
endif()

if(EDGE_BUILD_BENCHMARKS)
    add_executable(edge_ipc_bench bench/ipc_latency_bench.cpp)
    target_link_libraries(edge_ipc_bench PRIVATE edge_ipc edge_test_support)
    edge_configure_target(edge_ipc_bench)
endif()
