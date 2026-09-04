/**
 * @file config.h
 * @brief Typed startup configuration for the Gimbal Controller process.
 */
#pragma once

#include <cstdint>
#include <string>

namespace edge::gimbal_controller::runtime {

    /** @brief STANAG-over-UDP adapter endpoint configuration. */
    struct Stanag_Udp_Config {
        /** Destination IPv4 host/address for dummy STANAG datagrams. */
        std::string host;
        /** Destination UDP port. */
        std::uint16_t port;
    };

    /** @brief Complete runtime configuration consumed by Gimbal_Controller. */
    struct Gimbal_Controller_Config {
        /** ZeroMQ command REP endpoint. */
        std::string command_endpoint;
        /** Configuration for the currently selected STANAG output adapter. */
        Stanag_Udp_Config stanag_udp;
    };

    /**
     * @brief Reads Gimbal Controller configuration once at the runtime boundary.
     * @return Typed configuration with documented defaults applied.
     *
     * @details Invalid `EDGE_STANAG_UDP_PORT` values intentionally preserve legacy behavior and fall
     * back to the default port rather than failing process startup. Adapter code does not read the
     * environment directly.
     */
    [[nodiscard]] auto Load_Config() -> Gimbal_Controller_Config;

}  // namespace edge::gimbal_controller::runtime
