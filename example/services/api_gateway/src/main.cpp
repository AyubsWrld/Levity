/**
 * @file main.cpp
 * @brief Process entry point for the API Gateway executable.
 *
 * @details Source path: `services/api_gateway/src/main.cpp`.
 */

#include <edge/api_gateway/runtime/api_gateway.h>
#include <edge/api_gateway/runtime/config.h>
#include <edge/platform/logging/logger.h>
#include <edge/platform/process/shutdown_signal.h>

#include <cstdlib>
#include <exception>
#include <string>
#include <string_view>

namespace {
    /** Stable service identifier included in every structured log record. */
    constexpr std::string_view SERVICE_NAME = "api_gateway";
}

/**
 * @brief Loads configuration, owns process shutdown signaling, and runs the API Gateway lifecycle.
 * @return EXIT_SUCCESS after graceful SIGINT/SIGTERM shutdown; EXIT_FAILURE on startup or main-thread lifecycle
 * exception.
 */
auto main() -> int {
    // The outer try/catch has no Logger available yet, so it cannot log; it exists solely to
    // guarantee main() never lets an exception escape the process, even if Logger construction
    // itself somehow throws (e.g. std::bad_alloc).
    try {
        edge::platform::logging::Logger logger { std::string( SERVICE_NAME ) };

        try {
            // Construct before any worker threads so SIGINT/SIGTERM remain blocked in every service
            // thread and can be synchronously consumed by the main thread below.
            const edge::platform::process::Shutdown_Signal shutdown_signal;
            const auto config = edge::api_gateway::runtime::Load_Config();
            edge::api_gateway::runtime::Api_Gateway gateway( config, logger );

            logger.Info( "service_started",
                         { { "http_host", config.http.host },
                           { "http_port", std::to_string( gateway.Bound_Http_Port() ) },
                           { "telemetry_endpoint", config.telemetry_endpoint },
                           { "gimbal_command_endpoint", config.gimbal_command_endpoint } } );

            (void)shutdown_signal.Wait();

            logger.Info( "service_stopping" );
            gateway.Stop();
            logger.Info( "service_stopped" );
            return EXIT_SUCCESS;
        } catch( const std::exception& error ) {
            logger.Error( "startup_or_runtime_failure", { { "detail", error.what() } } );
            return EXIT_FAILURE;
        } catch( ... ) {
            logger.Error( "startup_or_runtime_failure", { { "detail", "non-standard exception" } } );
            return EXIT_FAILURE;
        }
    } catch( ... ) {
        return EXIT_FAILURE;
    }
}
