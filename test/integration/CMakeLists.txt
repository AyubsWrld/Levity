# Black-box multi-process Tier-1 integration suite. The service binaries are runtime dependencies,
# never linked into this target.

if(NOT EDGE_BUILD_TESTS)
    return()
endif()

if(NOT TARGET edge_test_support)
    add_subdirectory(
        ${CMAKE_CURRENT_SOURCE_DIR}/../support
        ${CMAKE_CURRENT_BINARY_DIR}/../support
    )
endif()
edge_add_test_executable(integration_tests
    LABEL integration
    SOURCES
        support/process_handle.cpp
        support/udp_listener.cpp
        support/stanag_wire.cpp
        support/test_harness.cpp
        slew_to_cue_test.cpp
        negative_paths_test.cpp
        restart_recovery_test.cpp
        fault_isolation_test.cpp
)

target_include_directories(integration_tests PRIVATE support)
target_link_libraries(integration_tests
    PRIVATE
        edge_schemas
        edge_ipc
        edge_platform
        edge_test_support
        httplib::httplib
        nlohmann_json::nlohmann_json
)

add_dependencies(integration_tests api_gateway gimbal_controller dummy_monolith)
target_compile_definitions(integration_tests PRIVATE
    EDGE_API_GATEWAY_BINARY="$<TARGET_FILE:api_gateway>"
    EDGE_GIMBAL_CONTROLLER_BINARY="$<TARGET_FILE:gimbal_controller>"
    EDGE_DUMMY_MONOLITH_BINARY="$<TARGET_FILE:dummy_monolith>"
)

set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/support/integration_test_properties.cmake")
