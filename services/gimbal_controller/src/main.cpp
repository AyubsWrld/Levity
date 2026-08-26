/**
 * @file main.cpp
 * @brief Process entry point for the Gimbal Controller executable.
 *
 * @details Source path: `services/gimbal_controller/src/main.cpp`.
 */

#include <edge/gimbal_controller/runtime/config.h>
#include <edge/gimbal_controller/runtime/gimbal_controller.h>
#include <edge/platform/logging/logger.h>
#include <edge/platform/process/shutdown_signal.h>

#include <cstdlib>
#include <exception>
#include <string>
#include <string_view>

namespace {
    /** Stable service identifier included in every structured log record. */
    constexpr std::string_view SERVICE_NAME = "gimbal_controller";
}

/**
 * @brief Loads configuration, starts the controller runtime, and waits synchronously for shutdown.
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
            const edge::platform::process::Shutdown_Signal shutdown_signal;
            const auto config = edge::gimbal_controller::runtime::Load_Config();
            edge::gimbal_controller::runtime::Gimbal_Controller controller( config, logger );

            logger.Info( "service_started",
                         { { "command_endpoint", config.command_endpoint },
                           { "stanag_udp_host", config.stanag_udp.host },
                           { "stanag_udp_port", std::to_string( config.stanag_udp.port ) } } );

            (void)shutdown_signal.Wait();

            logger.Info( "service_stopping" );
            controller.Stop();
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
