/**
 * @file requester.cpp
 * @brief Implementation of requester.
 *
 * @details Source path: `shared/libs/ipc/src/requester.cpp`.
 */

#include "detail/context_impl.h"
#include "detail/endpoint.h"
#include "detail/socket.h"

#include <edge/ipc/context.h>
#include <edge/ipc/limits.h>
#include <edge/ipc/requester.h>

#include <chrono>
#include <stdexcept>
#include <thread>
#include <utility>

namespace edge::ipc {

    /** @brief Native REQ state retained for timeout recovery and thread-affinity checks. */
    struct Requester::Impl {
        /** Owned native REQ socket. */
        zmq::socket_t socket;
        /** Non-owning native context pointer used when recreating a timed-out REQ socket. */
        zmq::context_t* context;
        /** Connected endpoint retained so protocol-state recovery can recreate the socket. */
        std::string endpoint;
        /** Thread allowed to use the native socket. */
        std::thread::id owning_thread;
    };

    /** @copydoc Requester::Requester */
    Requester::Requester( std::unique_ptr<Impl> impl ) : m_impl( std::move( impl ) ) {}
    /** @copydoc Requester::~Requester */
    Requester::~Requester() = default;
    /** @copydoc Requester::Requester */
    // NOLINTNEXTLINE(performance-noexcept-move-constructor, bugprone-exception-escape)
    Requester::Requester( Requester&& other ) {
        if( other.m_impl && other.m_impl->owning_thread != std::this_thread::get_id() ) {
            throw std::logic_error( "Requester cannot be moved from a thread other than its owner" );
        }
        m_impl = std::move( other.m_impl );
    }

    /** @copydoc Requester::Connect */
    auto Requester::Connect( Context& context, std::string_view endpoint ) -> edge::Result<Requester, Error> {
        if( auto validation = detail::Validate_Endpoint( endpoint ); !validation.has_value() ) {
            return edge::Make_Failure( validation.error() );
        }

        auto& native_context = context.Native_Impl().context;
        try {
            zmq::socket_t socket( native_context, zmq::socket_type::req );
            detail::Configure_New_Socket( socket );
            socket.connect( std::string( endpoint ) );
            return Requester( std::make_unique<Impl>(
                Impl { std::move( socket ), &native_context, std::string( endpoint ), std::this_thread::get_id() } ) );
        } catch( const zmq::error_t& error ) {
            return edge::Make_Failure( detail::Map_Connect_Error( error ) );
        }
    }

    /** @copydoc Requester::Recreate_Socket */
    auto Requester::Recreate_Socket() -> edge::Result<void, Error> {
        try {
            zmq::socket_t socket( *m_impl->context, zmq::socket_type::req );
            detail::Configure_New_Socket( socket );
            socket.connect( m_impl->endpoint );
            m_impl->socket = std::move( socket );
            return {};
        } catch( const zmq::error_t& error ) {
            return edge::Make_Failure( detail::Map_Connect_Error( error ) );
        }
    }

    /** @copydoc Requester::Request */
    auto Requester::Request( std::string_view payload, std::chrono::milliseconds timeout )
        -> edge::Result<std::string, Error> {
        if( std::this_thread::get_id() != m_impl->owning_thread ) {
            return edge::Make_Failure(
                Error { Error_Code::PROTOCOL_STATE_ERROR, "Requester used from a thread other than its owner" } );
        }

        if( payload.size() > MAX_SERIALIZED_MESSAGE_BYTES ) {
            return edge::Make_Failure( Error { Error_Code::MESSAGE_TOO_LARGE, "payload exceeds IPC message limit" } );
        }

        const auto total_budget = detail::Normalize_Timeout( timeout );
        const auto deadline = std::chrono::steady_clock::now() + total_budget;

        try {
            detail::Set_Send_Timeout( m_impl->socket, total_budget );
            if( !m_impl->socket.send( zmq::message_t( payload.data(), payload.size() ), zmq::send_flags::none ) ) {
                const auto recreated = Recreate_Socket();
                if( !recreated.has_value() ) {
                    return edge::Make_Failure( recreated.error() );
                }
                return edge::Make_Failure( Error { Error_Code::TIMEOUT, "request send timed out" } );
            }
        } catch( const zmq::error_t& error ) {
            const auto recreated = Recreate_Socket();
            if( !recreated.has_value() ) {
                return edge::Make_Failure( recreated.error() );
            }
            return edge::Make_Failure( Error { Error_Code::SEND_FAILED, error.what() } );
        }

        const auto now = std::chrono::steady_clock::now();
        auto receive_budget = std::chrono::milliseconds::zero();
        if( now < deadline ) {
            // Round a positive sub-millisecond remainder up rather than truncating it to a
            // zero-millisecond poll. The resulting native timeout can exceed the steady-clock
            // deadline by less than one millisecond, which is the transport's timer resolution.
            receive_budget = std::chrono::ceil<std::chrono::milliseconds>( deadline - now );
        }
        detail::Set_Receive_Timeout( m_impl->socket, receive_budget );

        zmq::message_t reply;
        try {
            const auto result = m_impl->socket.recv( reply, zmq::recv_flags::none );
            if( !result ) {
                const auto recreated = Recreate_Socket();
                if( !recreated.has_value() ) {
                    return edge::Make_Failure( recreated.error() );
                }
                return edge::Make_Failure( Error {
                    Error_Code::TIMEOUT, "reply timed out; command outcome is ambiguous and was not retried" } );
            }
        } catch( const zmq::error_t& error ) {
            const auto recreated = Recreate_Socket();
            if( !recreated.has_value() ) {
                return edge::Make_Failure( recreated.error() );
            }
            return edge::Make_Failure( Error { Error_Code::RECEIVE_FAILED, error.what() } );
        }

        if( reply.more() ) {
            const auto recreated = Recreate_Socket();
            if( !recreated.has_value() ) {
                return edge::Make_Failure( recreated.error() );
            }
            return edge::Make_Failure(
                Error { Error_Code::RECEIVE_FAILED, "malformed REQ/REP reply: unexpected extra frame" } );
        }

        if( reply.size() > MAX_SERIALIZED_MESSAGE_BYTES ) {
            return edge::Make_Failure( Error { Error_Code::MESSAGE_TOO_LARGE, "reply exceeds IPC message limit" } );
        }
        return std::string( static_cast<const char*>( reply.data() ), reply.size() );
    }

    /** @copydoc Requester::Create */
    auto Requester::Create( Context& context, std::string_view endpoint ) -> edge::Result<Requester, Error> {
        return Connect( context, endpoint );
    }

}  // namespace edge::ipc
