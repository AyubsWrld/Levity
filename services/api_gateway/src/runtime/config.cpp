/**
 * @file config.cpp
 * @brief Implementation of config.
 *
 * @details Source path: `services/api_gateway/src/runtime/config.cpp`.
 */

#include <edge/api_gateway/runtime/config.h>
#include <edge/ipc/endpoints.h>
#include <edge/platform/config/environment.h>

#include <stdexcept>
#include <string>
#include <string_view>

namespace edge::api_gateway::runtime {

    namespace {

        /** @brief Parses a required port and converts invalid text into a startup exception. */
        [[nodiscard]] auto Parse_Required_Port( std::string_view variable, std::string_view text ) -> std::uint16_t {
            const auto port = edge::platform::config::Parse_Port( text );
            if( !port.has_value() ) {
                throw std::runtime_error( std::string( variable ) + "='" + std::string( text ) +
                                          "' is not a valid port" );
            }
            return *port;
        }

    }  // namespace

    /** @copydoc edge::api_gateway::runtime::Load_Config() */
    auto Load_Config() -> Api_Gateway_Config {
        const auto http_port_text = edge::platform::config::Read_Environment( "EDGE_HTTP_PORT", "8080" );

        return Api_Gateway_Config {
            .telemetry_endpoint = edge::platform::config::Read_Environment( "EDGE_TELEMETRY_ENDPOINT",
                                                                            edge::ipc::DEFAULT_TELEMETRY_ENDPOINT ),
            .gimbal_command_endpoint = edge::platform::config::Read_Environment(
                "EDGE_GIMBAL_COMMAND_ENDPOINT", edge::ipc::DEFAULT_GIMBAL_COMMAND_ENDPOINT ),
            .http =
                Http_Config {
                    .host = edge::platform::config::Read_Environment( "EDGE_HTTP_HOST", "127.0.0.1" ),
                    .port = Parse_Required_Port( "EDGE_HTTP_PORT", http_port_text ),
                },
        };
    }

}  // namespace edge::api_gateway::runtime
