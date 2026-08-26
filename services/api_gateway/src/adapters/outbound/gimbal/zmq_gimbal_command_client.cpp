/**
 * @file zmq_gimbal_command_client.cpp
 * @brief Implementation of zmq gimbal command client.
 *
 * @details Source path: `services/api_gateway/src/adapters/outbound/gimbal/zmq_gimbal_command_client.cpp`.
 */

#include "commands.pb.h"

#include <edge/api_gateway/adapters/outbound/gimbal/zmq_gimbal_command_client.h>
#include <edge/ipc/error.h>
#include <edge/ipc/requester.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace edge::api_gateway::adapters::outbound::gimbal {

    namespace {

        constexpr auto QUEUE_POLL_INTERVAL = std::chrono::milliseconds( 100 );

        /** @brief Converts transport-layer IPC errors into the outbound port's stable error model. */
        [[nodiscard]] auto Map_Ipc_Error( const edge::ipc::Error& error )
            -> core::ports::outbound::Gimbal_Command_Error {
            using core::ports::outbound::Gimbal_Command_Error;
            using core::ports::outbound::Gimbal_Command_Error_Code;
            using edge::ipc::Error_Code;

            switch( error.code ) {
                case Error_Code::TIMEOUT:
                    return Gimbal_Command_Error { Gimbal_Command_Error_Code::TIMEOUT, error.detail };
                case Error_Code::PEER_UNAVAILABLE:
                    return Gimbal_Command_Error { Gimbal_Command_Error_Code::UNAVAILABLE, error.detail };
                case Error_Code::BIND_FAILED:
                case Error_Code::CONNECT_FAILED:
                case Error_Code::INVALID_ENDPOINT:
                case Error_Code::SEND_FAILED:
                case Error_Code::RECEIVE_FAILED:
                case Error_Code::MESSAGE_TOO_LARGE:
                case Error_Code::PROTOCOL_STATE_ERROR:
                case Error_Code::SHUTTING_DOWN:
                    return Gimbal_Command_Error { Gimbal_Command_Error_Code::TRANSPORT_ERROR, error.detail };
            }

            return Gimbal_Command_Error { Gimbal_Command_Error_Code::TRANSPORT_ERROR, "unrecognized IPC error" };
        }

    }  // namespace

    /** @copydoc Zmq_Gimbal_Command_Client::Zmq_Gimbal_Command_Client */
    Zmq_Gimbal_Command_Client::Zmq_Gimbal_Command_Client( edge::ipc::Context& context, std::string endpoint ) {
        auto ready_promise = std::make_shared<std::promise<Startup_Result>>();
        auto ready_future = ready_promise->get_future();

        m_worker = std::jthread(
            [this, &context, endpoint = std::move( endpoint ), ready_promise]( std::stop_token stop_token ) {
                Run( std::move( stop_token ), context, endpoint, *ready_promise );
            } );

        auto startup_result = ready_future.get();
        if( !startup_result.has_value() ) {
            m_worker.request_stop();
            m_worker.join();
            throw std::runtime_error( "Zmq_Gimbal_Command_Client: " + startup_result.error() );
        }
    }

    /** @copydoc Zmq_Gimbal_Command_Client::~Zmq_Gimbal_Command_Client */
    Zmq_Gimbal_Command_Client::~Zmq_Gimbal_Command_Client() { Stop(); }

    /** @copydoc Zmq_Gimbal_Command_Client::Stop */
    void Zmq_Gimbal_Command_Client::Stop() noexcept {
        {
            const std::scoped_lock lock( m_mutex );
            m_shutting_down = true;
        }
        m_queue_not_empty.notify_all();

        if( m_worker.joinable() ) {
            m_worker.request_stop();
            m_worker.join();
        }
    }

    /** @copydoc Zmq_Gimbal_Command_Client::Send_Slew */
    auto Zmq_Gimbal_Command_Client::Send_Slew( const edge::commands::v1::SlewCommand& command ) -> Reply_Result {
        std::promise<Reply_Result> promise;
        auto future = promise.get_future();
        const auto deadline = std::chrono::steady_clock::now() + CALLER_WAIT_TIMEOUT;

        {
            const std::scoped_lock lock( m_mutex );
            if( m_shutting_down ) {
                return edge::Make_Failure( core::ports::outbound::Gimbal_Command_Error {
                    core::ports::outbound::Gimbal_Command_Error_Code::UNAVAILABLE,
                    "command channel is shutting down" } );
            }
            if( m_queue.size() >= COMMAND_QUEUE_CAPACITY ) {
                return edge::Make_Failure( core::ports::outbound::Gimbal_Command_Error {
                    core::ports::outbound::Gimbal_Command_Error_Code::QUEUE_FULL, "command queue is full" } );
            }
            m_queue.push_back( Work_Item { command, deadline, std::move( promise ) } );
        }
        m_queue_not_empty.notify_one();

        if( future.wait_until( deadline ) != std::future_status::ready ) {
            return edge::Make_Failure( core::ports::outbound::Gimbal_Command_Error {
                core::ports::outbound::Gimbal_Command_Error_Code::TIMEOUT,
                "gateway did not receive a gimbal reply before the caller deadline; the outcome is ambiguous and "
                "was not retried" } );
        }

        return future.get();
    }

    /** @copydoc Zmq_Gimbal_Command_Client::Run */
    void Zmq_Gimbal_Command_Client::Run( std::stop_token stop_token,
                                         edge::ipc::Context& context,
                                         const std::string& endpoint,
                                         std::promise<Startup_Result>& ready_promise ) {
        auto requester_result = [&]() -> edge::Result<edge::ipc::Requester, std::string> {
            try {
                auto result = edge::ipc::Requester::Connect( context, endpoint );
                if( !result.has_value() ) {
                    return edge::Make_Failure( result.error().detail );
                }
                return std::move( *result );
            } catch( const std::exception& error ) {
                return edge::Make_Failure( std::string( "requester startup exception: " ) + error.what() );
            } catch( ... ) {
                return edge::Make_Failure( std::string( "requester startup failed with an unknown exception" ) );
            }
        }();
        if( !requester_result.has_value() ) {
            ready_promise.set_value( edge::Make_Failure( requester_result.error() ) );
            return;
        }
        ready_promise.set_value( Startup_Result {} );

        auto& requester = *requester_result;
        while( true ) {
            std::optional<Work_Item> item;
            {
                std::unique_lock lock( m_mutex );
                m_queue_not_empty.wait_for(
                    lock, QUEUE_POLL_INTERVAL, [&] { return !m_queue.empty() || stop_token.stop_requested(); } );

                if( stop_token.stop_requested() ) {
                    while( !m_queue.empty() ) {
                        auto pending = std::move( m_queue.front() );
                        m_queue.pop_front();
                        pending.promise.set_value( edge::Make_Failure( core::ports::outbound::Gimbal_Command_Error {
                            core::ports::outbound::Gimbal_Command_Error_Code::TRANSPORT_ERROR,
                            "gateway is shutting down" } ) );
                    }
                    return;
                }

                if( !m_queue.empty() ) {
                    item = std::move( m_queue.front() );
                    m_queue.pop_front();
                }
            }

            if( !item.has_value() ) {
                continue;
            }

            const auto now = std::chrono::steady_clock::now();
            if( now >= item->deadline ) {
                item->promise.set_value( edge::Make_Failure( core::ports::outbound::Gimbal_Command_Error {
                    core::ports::outbound::Gimbal_Command_Error_Code::TIMEOUT,
                    "command expired in the local queue and was not sent" } ) );
                continue;
            }

            std::string request_bytes;
            if( !item->command.SerializeToString( &request_bytes ) ) {
                item->promise.set_value( edge::Make_Failure( core::ports::outbound::Gimbal_Command_Error {
                    core::ports::outbound::Gimbal_Command_Error_Code::TRANSPORT_ERROR,
                    "failed to serialize SlewCommand" } ) );
                continue;
            }

            const auto remaining =
                std::chrono::ceil<std::chrono::milliseconds>( item->deadline - std::chrono::steady_clock::now() );
            const auto request_timeout = std::min( GIMBAL_REQUEST_TIMEOUT, remaining );
            if( request_timeout <= std::chrono::milliseconds::zero() ) {
                item->promise.set_value( edge::Make_Failure( core::ports::outbound::Gimbal_Command_Error {
                    core::ports::outbound::Gimbal_Command_Error_Code::TIMEOUT,
                    "command expired before transport send and was not sent" } ) );
                continue;
            }

            auto request_result = requester.Request( request_bytes, request_timeout );
            if( !request_result.has_value() ) {
                item->promise.set_value( edge::Make_Failure( Map_Ipc_Error( request_result.error() ) ) );
                continue;
            }

            edge::commands::v1::SlewCommandReply reply;
            if( !reply.ParseFromString( *request_result ) ) {
                item->promise.set_value( edge::Make_Failure( core::ports::outbound::Gimbal_Command_Error {
                    core::ports::outbound::Gimbal_Command_Error_Code::TRANSPORT_ERROR,
                    "malformed SlewCommandReply from gimbal" } ) );
                continue;
            }

            item->promise.set_value( std::move( reply ) );
        }
    }

}  // namespace edge::api_gateway::adapters::outbound::gimbal
