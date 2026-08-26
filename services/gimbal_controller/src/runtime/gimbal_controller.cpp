/**
 * @file gimbal_controller.cpp
 * @brief Implementation of gimbal controller.
 *
 * @details Source path: `services/gimbal_controller/src/runtime/gimbal_controller.cpp`.
 */

#include <edge/gimbal_controller/adapters/inbound/commands/zmq_command_server.h>
#include <edge/gimbal_controller/adapters/outbound/stanag/stanag_udp_sender.h>
#include <edge/gimbal_controller/core/services/gimbal_service.h>
#include <edge/gimbal_controller/runtime/gimbal_controller.h>
#include <edge/ipc/context.h>

namespace edge::gimbal_controller::runtime {

    /**
     * @brief Concrete Gimbal Controller dependency graph.
     * @details Declaration order guarantees the selected output adapter outlives Gimbal_Service and
     * the core service outlives the inbound command server that references it.
     */
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    // Impl is a private pimpl-style composition-root aggregate: it is declared only in this
    // translation unit and accessed solely through the owning Gimbal_Controller, so there is no
    // external caller to encapsulate against.
    struct Gimbal_Controller::Impl {
        /** Shared IPC context; declared first so it outlives the command server. */
        edge::ipc::Context ipc_context;

        /** Selected concrete hardware/output adapter implementing Gimbal_Output_Port. */
        adapters::outbound::stanag::Stanag_Udp_Sender gimbal_output;

        /** Transport-independent application service implementing Command_Port. */
        core::services::Gimbal_Service gimbal_service;

        /** Concrete inbound command transport targeting Command_Port only. */
        adapters::inbound::commands::Zmq_Command_Server command_server;

        /** @brief Constructs the selected output adapter, core service, then inbound server. */
        Impl( const Gimbal_Controller_Config& config, edge::platform::logging::Logger& logger )
            : gimbal_output( config.stanag_udp.host, config.stanag_udp.port ),
              gimbal_service( gimbal_output ),
              command_server( ipc_context, config.command_endpoint, gimbal_service, logger ) {}

        /** @brief Stops inbound command processing before dependent core/output objects are destroyed. */
        void Stop() noexcept { command_server.Stop(); }
    };
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    /** @copydoc Gimbal_Controller::Gimbal_Controller */
    Gimbal_Controller::Gimbal_Controller( const Gimbal_Controller_Config& config,
                                          edge::platform::logging::Logger& logger )
        : m_impl( std::make_unique<Impl>( config, logger ) ) {}

    /** @copydoc Gimbal_Controller::~Gimbal_Controller */
    Gimbal_Controller::~Gimbal_Controller() { Stop(); }

    /** @copydoc Gimbal_Controller::Stop */
    void Gimbal_Controller::Stop() noexcept {
        if( m_impl ) {
            m_impl->Stop();
        }
    }

}  // namespace edge::gimbal_controller::runtime
