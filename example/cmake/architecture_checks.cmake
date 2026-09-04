# Configure-time enforcement of the Hexagonal dependency direction.
#
# The build graph is the mechanical guard against architectural drift:
#   * api_gateway and gimbal_controller must never depend on each other's targets;
#   * core targets must not depend on HTTP / ZeroMQ / UDP infrastructure packages;
#   * core targets must not depend on adapter targets (the dependency points inward);
#   * the generated Protobuf domain contract (edge_schemas) is the one sanctioned core dependency.
#
# Call edge_verify_architecture() from the root CMakeLists.txt after every subdirectory is added.

set(EDGE_CORE_TARGETS api_gateway_core gimbal_core)

set(EDGE_FORBIDDEN_CORE_DEPENDENCY_PATTERNS
    "httplib"
    "cppzmq"
    "libzmq"
    "ZeroMQ"
    "nlohmann_json"
    "_adapters$"
    "edge_ipc")

function(_edge_collect_link_libraries target out_var)
    set(collected "")

    foreach(property IN ITEMS LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
        get_target_property(value ${target} ${property})

        if(value)
            list(APPEND collected ${value})
        endif()
    endforeach()

    if(collected)
        list(REMOVE_DUPLICATES collected)
    endif()

    set(${out_var} "${collected}" PARENT_SCOPE)
endfunction()

function(edge_verify_architecture)
    # 1. Core targets must not reach infrastructure or adapters.
    foreach(core_target IN LISTS EDGE_CORE_TARGETS)
        if(NOT TARGET ${core_target})
            continue()
        endif()

        _edge_collect_link_libraries(${core_target} core_dependencies)

        foreach(dependency IN LISTS core_dependencies)
            foreach(pattern IN LISTS EDGE_FORBIDDEN_CORE_DEPENDENCY_PATTERNS)
                if(dependency MATCHES "${pattern}")
                    message(FATAL_ERROR
                        "Architecture violation: core target '${core_target}' links '${dependency}'. "
                        "Core/application logic must not depend on HTTP, ZeroMQ or adapter targets.")
                endif()
            endforeach()
        endforeach()
    endforeach()

    # 2. The two services must remain mutually independent.
    set(gateway_targets api_gateway_core api_gateway_adapters api_gateway api_gateway_tests)
    set(gimbal_targets gimbal_core gimbal_adapters gimbal_controller gimbal_controller_tests)

    foreach(gateway_target IN LISTS gateway_targets)
        if(TARGET ${gateway_target})
            _edge_collect_link_libraries(${gateway_target} gateway_dependencies)

            foreach(dependency IN LISTS gateway_dependencies)
                if(dependency MATCHES "^gimbal")
                    message(FATAL_ERROR
                        "Architecture violation: '${gateway_target}' links '${dependency}'. "
                        "api_gateway must never depend on gimbal_controller code.")
                endif()
            endforeach()
        endif()
    endforeach()

    foreach(gimbal_target IN LISTS gimbal_targets)
        if(TARGET ${gimbal_target})
            _edge_collect_link_libraries(${gimbal_target} gimbal_dependencies)

            foreach(dependency IN LISTS gimbal_dependencies)
                if(dependency MATCHES "^api_gateway")
                    message(FATAL_ERROR
                        "Architecture violation: '${gimbal_target}' links '${dependency}'. "
                        "gimbal_controller must never depend on api_gateway code.")
                endif()
            endforeach()
        endif()
    endforeach()

    # 3. The black-box integration suite must not link service code.
    if(TARGET integration_tests)
        _edge_collect_link_libraries(integration_tests integration_dependencies)

        foreach(dependency IN LISTS integration_dependencies)
            if(dependency MATCHES "^api_gateway|^gimbal_core|^gimbal_adapters|^gimbal_controller")
                message(FATAL_ERROR
                    "Architecture violation: integration_tests links service target '${dependency}'. "
                    "Integration tests must exercise the built executables as black boxes.")
            endif()
        endforeach()
    endif()

    message(STATUS "Architecture checks passed: service isolation and core/adapter direction hold.")
endfunction()
