/**
 * @file zmq_command_server.cpp
 * @brief Implementation of zmq command server.
 *
 * @details Source path: `services/gimbal_controller/src/adapters/inbound/commands/zmq_command_server.cpp`.
 */

#include "commands.pb.h"

#include <edge/gimbal_controller/adapters/inbound/commands/zmq_command_server.h>
#include <edge/ipc/replier.h>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>

namespace edge::gimbal_controller::adapters::inbound::commands {

    namespace {
        /** Bounded REP receive interval used to re-check the worker stop token. */
        constexpr auto COMMAND_RECEIVE_TIMEOUT = std::chrono::milliseconds( 100 );

        /** @brief Maps domain reply statuses to bounded structured-log tokens. */
        [[nodiscard]] auto Status_Name( edge::commands::v1::SlewStatus status ) -> std::string_view {
            switch( status ) {
                case edge::commands::v1::SLEW_STATUS_ACCEPTED:
                    return "accepted";
                case edge::commands::v1::SLEW_STATUS_REJECTED_INVALID:
                    return "rejected_invalid";
                case edge::commands::v1::SLEW_STATUS_HARDWARE_ERROR:
                    return "hardware_error";
                default:
                    return "unspecified";
            }
        }
    }  // namespace

    /** @copydoc Zmq_Command_Server::Zmq_Command_Server */
    Zmq_Command_Server::Zmq_Command_Server( edge::ipc::Context& context,
                                            std::string endpoint,
                                            core::ports::inbound::Command_Port& commands,
                                            edge::platform::logging::Logger& logger ) {
        auto ready_promise = std::make_shared<std::promise<Startup_Result>>();
        auto ready_future = ready_promise->get_future();

        m_worker = std::jthread( [&context, endpoint = std::move( endpoint ), &commands, &logger, ready_promise](
                                     const std::stop_token& stop_token ) {
            Run( context, endpoint, commands, logger, stop_token, *ready_promise );
        } );

        auto startup_result = ready_future.get();
        if( !startup_result.has_value() ) {
            m_worker.request_stop();
            m_worker.join();
            throw std::runtime_error( "Zmq_Command_Server: " + startup_result.error() );
        }
    }

    /** @copydoc Zmq_Command_Server::~Zmq_Command_Server */
    Zmq_Command_Server::~Zmq_Command_Server() { Stop(); }

    /** @copydoc Zmq_Command_Server::Stop */
    void Zmq_Command_Server::Stop() noexcept {
        if( m_worker.joinable() ) {
            m_worker.request_stop();
            m_worker.join();
        }
    }

    /** @copydoc Zmq_Command_Server::Run */
    void Zmq_Command_Server::Run( edge::ipc::Context& context,
                                  const std::string& endpoint,
                                  core::ports::inbound::Command_Port& commands,
                                  edge::platform::logging::Logger& logger,
                                  const std::stop_token& stop_token,
                                  std::promise<Startup_Result>& ready_promise ) {
        auto replier_result = [&]() -> edge::Result<edge::ipc::Replier, std::string> {
            try {
                auto result = edge::ipc::Replier::Bind( context, endpoint );
                if( !result.has_value() ) {
                    return edge::Make_Failure( result.error().detail );
                }
                return std::move( *result );
            } catch( const std::exception& error ) {
                return edge::Make_Failure( std::string( "replier startup exception: " ) + error.what() );
            } catch( ... ) {
                return edge::Make_Failure( std::string( "replier startup failed with an unknown exception" ) );
            }
        }();
        if( !replier_result.has_value() ) {
            ready_promise.set_value( edge::Make_Failure( replier_result.error() ) );
            return;
        }
        ready_promise.set_value( Startup_Result {} );

        auto& replier = *replier_result;
        while( !stop_token.stop_requested() ) {
            auto receive_result = replier.Receive( COMMAND_RECEIVE_TIMEOUT );
            if( !receive_result.has_value() ) {
                logger.Warn( "command_receive_failed", { { "detail", receive_result.error().detail } } );
                continue;
            }
            if( !receive_result->has_value() ) {
                continue;
            }

            auto& pending = **receive_result;  // NOLINT(bugprone-unchecked-optional-access) - checked above
            edge::commands::v1::SlewCommand command;
            edge::commands::v1::SlewCommandReply reply;

            const auto payload = pending.Payload();
            if( !command.ParseFromArray( payload.data(), static_cast<int>( payload.size() ) ) ) {
                reply.set_status( edge::commands::v1::SLEW_STATUS_REJECTED_INVALID );
                reply.set_detail( "malformed SlewCommand payload" );
                logger.Warn( "slew_command_rejected", { { "result", "malformed_payload" } } );
            } else {
                reply = commands.Handle_Slew_Command( command );
                logger.Info( "slew_command_handled",
                             { { "target_uid", command.target_uid() },
                               { "result", std::string( Status_Name( reply.status() ) ) } } );
            }

            std::string serialized_reply;
            if( !reply.SerializeToString( &serialized_reply ) ) {
                serialized_reply.clear();
            }

            if( auto reply_result = pending.Reply( serialized_reply ); !reply_result.has_value() ) {
                logger.Warn( "command_reply_failed", { { "detail", reply_result.error().detail } } );
            }
        }
    }

}  // namespace edge::gimbal_controller::adapters::inbound::commands
