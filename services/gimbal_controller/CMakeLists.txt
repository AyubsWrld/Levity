# gimbal_controller: same core/adapters/runtime/main service template as api_gateway.
# Core owns inbound/outbound ports; adapters implement or drive those ports; runtime wires concrete adapters.

add_library(gimbal_core STATIC
    src/core/services/gimbal_service.cpp
)
target_include_directories(gimbal_core PUBLIC include)
target_link_libraries(gimbal_core PUBLIC edge_schemas edge_common)

edge_configure_target(gimbal_core)
add_library(gimbal_controller_core ALIAS gimbal_core)

add_library(gimbal_controller_adapter_stanag STATIC
    src/adapters/outbound/stanag/stanag_encoder.cpp
    src/adapters/outbound/stanag/stanag_udp_sender.cpp
)
target_include_directories(gimbal_controller_adapter_stanag PUBLIC include)
target_link_libraries(gimbal_controller_adapter_stanag
    PUBLIC
        gimbal_controller_core
        edge_platform
)

edge_configure_target(gimbal_controller_adapter_stanag)

add_library(gimbal_controller_adapter_commands_zmq STATIC
    src/adapters/inbound/commands/zmq_command_server.cpp
)
target_include_directories(gimbal_controller_adapter_commands_zmq PUBLIC include)
target_link_libraries(gimbal_controller_adapter_commands_zmq
    PUBLIC
        gimbal_controller_core
        edge_platform
    PRIVATE
        edge_ipc
)

edge_configure_target(gimbal_controller_adapter_commands_zmq)

# Compatibility aggregate for repository-level architecture checks and existing consumers. New code
# should depend on the narrow adapter target it actually uses.
add_library(gimbal_adapters INTERFACE)
target_link_libraries(gimbal_adapters
    INTERFACE
        gimbal_controller_adapter_stanag
        gimbal_controller_adapter_commands_zmq
)

add_library(gimbal_controller_runtime STATIC
    src/runtime/config.cpp
    src/runtime/gimbal_controller.cpp
)
target_include_directories(gimbal_controller_runtime PUBLIC include)
target_link_libraries(gimbal_controller_runtime
    PUBLIC
        edge_platform
    PRIVATE
        gimbal_controller_core
        gimbal_controller_adapter_stanag
        gimbal_controller_adapter_commands_zmq
        edge_ipc
)

edge_configure_target(gimbal_controller_runtime)

add_executable(gimbal_controller src/main.cpp)
target_link_libraries(gimbal_controller PRIVATE gimbal_controller_runtime edge_platform)

edge_configure_target(gimbal_controller)

if(EDGE_BUILD_TESTS)
    if(NOT TARGET edge_test_support)
        add_subdirectory(
            ${CMAKE_CURRENT_SOURCE_DIR}/../../test/support
            ${CMAKE_BINARY_DIR}/test/support
        )
    endif()

    edge_add_test_executable(gimbal_controller_tests
        LABEL unit
        SOURCES
            test/core/gimbal_service_test.cpp
            test/adapters/inbound/commands/zmq_command_server_test.cpp
            test/adapters/outbound/stanag/stanag_encoder_test.cpp
            test/adapters/outbound/stanag/stanag_udp_sender_test.cpp
            test/runtime/config_test.cpp
    )
    target_link_libraries(gimbal_controller_tests
        PRIVATE
            gimbal_controller_core
            gimbal_controller_adapter_stanag
            gimbal_controller_adapter_commands_zmq
            gimbal_controller_runtime
            edge_ipc
            edge_platform
            edge_test_support
    )
endif()
