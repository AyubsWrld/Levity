/**
 * @file config.cpp
 * @brief Implementation of config.
 *
 * @details Source path: `services/gimbal_controller/src/runtime/config.cpp`.
 */

#include <edge/gimbal_controller/runtime/config.h>
#include <edge/ipc/endpoints.h>
#include <edge/platform/config/environment.h>

#include <string>

namespace edge::gimbal_controller::runtime {

    namespace {

        /** Legacy-compatible UDP destination port used when configuration is absent/invalid. */
        constexpr std::uint16_t DEFAULT_STANAG_UDP_PORT = 14550;

    }  // namespace

    /** @copydoc edge::gimbal_controller::runtime::Load_Config() */
    auto Load_Config() -> Gimbal_Controller_Config {
        const auto udp_port_text = edge::platform::config::Read_Environment( "EDGE_STANAG_UDP_PORT", "14550" );

        return Gimbal_Controller_Config {
            .command_endpoint = edge::platform::config::Read_Environment( "EDGE_GIMBAL_COMMAND_ENDPOINT",
                                                                          edge::ipc::DEFAULT_GIMBAL_COMMAND_ENDPOINT ),
            .stanag_udp =
                Stanag_Udp_Config {
                    .host = edge::platform::config::Read_Environment( "EDGE_STANAG_UDP_HOST", "127.0.0.1" ),
                    .port = edge::platform::config::Parse_Port( udp_port_text ).value_or( DEFAULT_STANAG_UDP_PORT ),
                },
        };
    }

}  // namespace edge::gimbal_controller::runtime
