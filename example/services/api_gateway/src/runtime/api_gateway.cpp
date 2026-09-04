/**
 * @file api_gateway.cpp
 * @brief Implementation of api gateway.
 *
 * @details Source path: `services/api_gateway/src/runtime/api_gateway.cpp`.
 */

#include <edge/api_gateway/adapters/inbound/rest/rest_server.h>
#include <edge/api_gateway/adapters/inbound/telemetry/zmq_telemetry_subscriber.h>
#include <edge/api_gateway/adapters/outbound/gimbal/zmq_gimbal_command_client.h>
#include <edge/api_gateway/core/services/api_service.h>
#include <edge/api_gateway/core/services/telemetry_service.h>
#include <edge/api_gateway/core/state/cue_cache.h>
#include <edge/api_gateway/runtime/api_gateway.h>
#include <edge/ipc/context.h>

namespace edge::api_gateway::runtime {

    /**
     * @brief Concrete API Gateway dependency graph owned by the runtime composition root.
     * @details Member declaration order encodes lifetime dependencies: context/state/outbound adapter
     * precede core services, which precede inbound adapters that reference them.
     */
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    // Impl is a private pimpl-style composition-root aggregate: it is declared only in this
    // translation unit and accessed solely through the owning Api_Gateway, so there is no external
    // caller to encapsulate against.
    struct Api_Gateway::Impl {
        /** Shared IPC context; declared first so it outlives all IPC adapters. */
        edge::ipc::Context ipc_context;

        /** Thread-safe core telemetry state. */
        core::state::Cue_Cache cue_cache;
        /** Concrete outbound gimbal adapter implementing Gimbal_Command_Port. */
        adapters::outbound::gimbal::Zmq_Gimbal_Command_Client gimbal_command_client;

        /** Core API service implementing the operator-facing Api_Port. */
        core::services::Api_Service api_service;

        /** Core telemetry service implementing the telemetry-ingestion inbound port. */
        core::services::Telemetry_Service telemetry_service;

        /** Inbound ZeroMQ telemetry adapter targeting Telemetry_Port only. */
        adapters::inbound::telemetry::Zmq_Telemetry_Subscriber telemetry_subscriber;

        /** Inbound REST adapter targeting Api_Port only. */
        adapters::inbound::rest::Rest_Server rest_server;

        /** @brief Constructs the dependency graph in lifetime-safe dependency order. */
        Impl( const Api_Gateway_Config& config, edge::platform::logging::Logger& logger )
            : gimbal_command_client( ipc_context, config.gimbal_command_endpoint ),
              api_service( cue_cache, gimbal_command_client ),
              telemetry_service( cue_cache ),
              telemetry_subscriber( ipc_context, config.telemetry_endpoint, telemetry_service, logger ),
              rest_server( config.http.host, config.http.port, api_service, logger ) {}

        /** @brief Stops inbound work first, then background/outbound workers in reverse-use order. */
        void Stop() noexcept {
            rest_server.Stop();
            telemetry_subscriber.Stop();
            gimbal_command_client.Stop();
        }
    };
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    /** @copydoc Api_Gateway::Api_Gateway */
    Api_Gateway::Api_Gateway( const Api_Gateway_Config& config, edge::platform::logging::Logger& logger )
        : m_impl( std::make_unique<Impl>( config, logger ) ) {}

    /** @copydoc Api_Gateway::~Api_Gateway */
    Api_Gateway::~Api_Gateway() { Stop(); }

    /** @copydoc Api_Gateway::Bound_Http_Port */
    auto Api_Gateway::Bound_Http_Port() const noexcept -> std::uint16_t { return m_impl->rest_server.Bound_Port(); }

    /** @copydoc Api_Gateway::Stop */
    void Api_Gateway::Stop() noexcept {
        if( m_impl ) {
            m_impl->Stop();
        }
    }

}  // namespace edge::api_gateway::runtime
