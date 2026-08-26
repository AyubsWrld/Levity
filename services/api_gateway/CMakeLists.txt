# api_gateway: mirrored service template
#   core/      - inbound/outbound ports, application services, state
#   adapters/  - inbound/outbound protocol implementations
#   runtime/   - typed configuration + composition/lifecycle root
#   main.cpp   - process bootstrap only

add_library(api_gateway_core STATIC
    src/core/state/cue_cache.cpp
    src/core/services/api_service.cpp
    src/core/services/telemetry_service.cpp
)
target_include_directories(api_gateway_core PUBLIC include)
target_link_libraries(api_gateway_core PUBLIC edge_schemas edge_common)

edge_configure_target(api_gateway_core)

add_library(api_gateway_adapter_rest STATIC
    src/adapters/inbound/rest/rest_server.cpp
)
target_include_directories(api_gateway_adapter_rest PUBLIC include)
target_link_libraries(api_gateway_adapter_rest
    PUBLIC
        api_gateway_core
        edge_platform
    PRIVATE
        httplib::httplib
        nlohmann_json::nlohmann_json
)

edge_configure_target(api_gateway_adapter_rest)

add_library(api_gateway_adapter_telemetry_zmq STATIC
    src/adapters/inbound/telemetry/zmq_telemetry_subscriber.cpp
)
target_include_directories(api_gateway_adapter_telemetry_zmq PUBLIC include)
target_link_libraries(api_gateway_adapter_telemetry_zmq
    PUBLIC
        api_gateway_core
        edge_platform
    PRIVATE
        edge_ipc
)

edge_configure_target(api_gateway_adapter_telemetry_zmq)

add_library(api_gateway_adapter_gimbal_zmq STATIC
    src/adapters/outbound/gimbal/zmq_gimbal_command_client.cpp
)
target_include_directories(api_gateway_adapter_gimbal_zmq PUBLIC include)
target_link_libraries(api_gateway_adapter_gimbal_zmq
    PUBLIC
        api_gateway_core
    PRIVATE
        edge_ipc
)

edge_configure_target(api_gateway_adapter_gimbal_zmq)

# Compatibility aggregate for repository-level architecture checks and existing consumers. New code
# should depend on the narrow adapter target it actually uses.
add_library(api_gateway_adapters INTERFACE)
target_link_libraries(api_gateway_adapters
    INTERFACE
        api_gateway_adapter_rest
        api_gateway_adapter_telemetry_zmq
        api_gateway_adapter_gimbal_zmq
)

add_library(api_gateway_runtime STATIC
    src/runtime/config.cpp
    src/runtime/api_gateway.cpp
)
target_include_directories(api_gateway_runtime PUBLIC include)
target_link_libraries(api_gateway_runtime
    PUBLIC
        edge_platform
    PRIVATE
        api_gateway_core
        api_gateway_adapter_rest
        api_gateway_adapter_telemetry_zmq
        api_gateway_adapter_gimbal_zmq
        edge_ipc
)

edge_configure_target(api_gateway_runtime)

add_executable(api_gateway src/main.cpp)
target_link_libraries(api_gateway PRIVATE api_gateway_runtime edge_platform)

edge_configure_target(api_gateway)

if(EDGE_BUILD_TESTS)
    if(NOT TARGET edge_test_support)
        add_subdirectory(
            ${CMAKE_CURRENT_SOURCE_DIR}/../../test/support
            ${CMAKE_BINARY_DIR}/test/support
        )
    endif()

    edge_add_test_executable(api_gateway_tests
        LABEL unit
        SOURCES
            test/core/cue_cache_test.cpp
            test/core/api_service_test.cpp
            test/core/telemetry_service_test.cpp
            test/adapters/inbound/rest/rest_server_test.cpp
            test/adapters/inbound/telemetry/zmq_telemetry_subscriber_test.cpp
            test/adapters/outbound/gimbal/zmq_gimbal_command_client_test.cpp
            test/runtime/config_test.cpp
    )
    target_link_libraries(api_gateway_tests
        PRIVATE
            api_gateway_core
            api_gateway_adapter_rest
            api_gateway_adapter_telemetry_zmq
            api_gateway_adapter_gimbal_zmq
            api_gateway_runtime
            edge_ipc
            edge_platform
            edge_test_support
            httplib::httplib
            nlohmann_json::nlohmann_json
    )
endif()
