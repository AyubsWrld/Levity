/**
 * @file config.h
 * @brief Typed startup configuration for the API Gateway process.
 */
#pragma once

#include <cstdint>
#include <string>

namespace edge::api_gateway::runtime {

    /** @brief REST listener configuration resolved at process startup. */
    struct Http_Config {
        /** Interface/address passed to the HTTP server bind operation. */
        std::string host;
        /** TCP port to bind; zero may be used by tests when supported by the adapter. */
        std::uint16_t port;
    };

    /**
     * @brief Complete immutable startup configuration consumed by Api_Gateway.
     *
     * @details Environment/config-file discovery is intentionally outside adapters. Runtime resolves
     * service-level values once and validates policy owned here (such as the HTTP port); individual
     * transport adapters still validate transport-specific values such as ZeroMQ endpoint syntax.
     */
    struct Api_Gateway_Config {
        /** ZeroMQ telemetry SUB endpoint. */
        std::string telemetry_endpoint;
        /** ZeroMQ gimbal-command REQ endpoint. */
        std::string gimbal_command_endpoint;
        /** REST listener settings. */
        Http_Config http;
    };

    /**
     * @brief Reads and validates API Gateway process configuration from the environment.
     *
     * @return Fully resolved typed configuration with defaults applied.
     * @throws std::runtime_error when the configured HTTP port is syntactically invalid or outside
     * the accepted range. Transport-specific configuration failures are reported by adapter
     * construction. Configuration failure is a startup failure, not a routine operational error.
     */
    [[nodiscard]] auto Load_Config() -> Api_Gateway_Config;

}  // namespace edge::api_gateway::runtime
