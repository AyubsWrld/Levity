/**
 * @file replier.cpp
 * @brief Implementation of replier.
 *
 * @details Source path: `shared/libs/ipc/src/replier.cpp`.
 */

#include "detail/context_impl.h"
#include "detail/endpoint.h"
#include "detail/socket.h"

#include <edge/ipc/context.h>
#include <edge/ipc/limits.h>
#include <edge/ipc/replier.h>

#include <stdexcept>
#include <thread>
#include <utility>

namespace edge::ipc {

    /** @brief Native REP state, bind metadata, owning thread, and request-cycle state flag. */
    struct Replier::Impl {
        /** Owned native REP socket. */
        zmq::socket_t socket;
        /** Non-owning native context used to recreate invalidated REP state. */
        zmq::context_t* context;
        /** Bound endpoint retained for socket-state recovery. */
        std::string endpoint;
        /** Thread allowed to perform REP operations. */
        std::thread::id owning_thread;
        /** True between successful Receive() and completion/abandonment of its Pending_Request. */
        bool awaiting_reply = false;
    };

    /** @copydoc Pending_Request::Pending_Request */
    Pending_Request::Pending_Request( Replier& owner, std::string payload )
        : m_owner( &owner ), m_payload( std::move( payload ) ) {}
    /** @copydoc Pending_Request::~Pending_Request */
    Pending_Request::~Pending_Request() = default;

    /** @copydoc Pending_Request::Pending_Request */
    Pending_Request::Pending_Request( Pending_Request&& other ) noexcept
        : m_owner( other.m_owner ), m_payload( std::move( other.m_payload ) ), m_replied( other.m_replied ) {
        other.m_owner = nullptr;
        other.m_replied = true;
    }

    /** @copydoc Pending_Request::Payload */
    auto Pending_Request::Payload() const noexcept -> std::string_view { return m_payload; }

    /** @copydoc Pending_Request::Reply */
    auto Pending_Request::Reply( std::string_view payload ) -> edge::Result<void, Error> {
        if( m_replied || m_owner == nullptr ) {
            return edge::Make_Failure( Error { Error_Code::PROTOCOL_STATE_ERROR, "request was already replied to" } );
        }
        if( payload.size() > MAX_SERIALIZED_MESSAGE_BYTES ) {
            return edge::Make_Failure( Error { Error_Code::MESSAGE_TOO_LARGE, "reply exceeds IPC message limit" } );
        }
        if( !m_owner->Is_Owning_Thread() ) {
            return edge::Make_Failure( Error { Error_Code::PROTOCOL_STATE_ERROR,
                                               "Pending_Request replied from a thread other than its owner" } );
        }

        m_replied = true;
        auto result = m_owner->Send_Reply( payload );
        if( !result.has_value() ) {
            const auto recreated = m_owner->Recreate_Socket();
            if( !recreated.has_value() ) {
                return edge::Make_Failure( recreated.error() );
            }
            return result;
        }

        m_owner->Clear_Awaiting_Reply();
        return {};
    }

    /** @copydoc Replier::Replier */
    Replier::Replier( std::unique_ptr<Impl> impl ) : m_impl( std::move( impl ) ) {}
    /** @copydoc Replier::~Replier */
    Replier::~Replier() = default;
    /** @copydoc Replier::Replier */
    // NOLINTNEXTLINE(performance-noexcept-move-constructor, bugprone-exception-escape)
    Replier::Replier( Replier&& other ) {
        if( other.m_impl && other.m_impl->owning_thread != std::this_thread::get_id() ) {
            throw std::logic_error( "Replier cannot be moved from a thread other than its owner" );
        }
        if( other.m_impl && other.m_impl->awaiting_reply ) {
            throw std::logic_error( "Replier cannot be moved while a Pending_Request is live" );
        }
        m_impl = std::move( other.m_impl );
    }

    /** @copydoc Replier::Bind */
    auto Replier::Bind( Context& context, std::string_view endpoint ) -> edge::Result<Replier, Error> {
        if( auto prepared = detail::Prepare_Bind_Endpoint( endpoint ); !prepared.has_value() ) {
            return edge::Make_Failure( prepared.error() );
        }

        auto& native_context = context.Native_Impl().context;
        try {
            zmq::socket_t socket( native_context, zmq::socket_type::rep );
            detail::Configure_New_Socket( socket );
            socket.bind( std::string( endpoint ) );
            return Replier( std::make_unique<Impl>( Impl {
                std::move( socket ), &native_context, std::string( endpoint ), std::this_thread::get_id(), false } ) );
        } catch( const zmq::error_t& error ) {
            return edge::Make_Failure( detail::Map_Bind_Error( error ) );
        }
    }

    /** @copydoc Replier::Recreate_Socket */
    auto Replier::Recreate_Socket() -> edge::Result<void, Error> {
        if( !Is_Owning_Thread() ) {
            return edge::Make_Failure( Error { Error_Code::PROTOCOL_STATE_ERROR,
                                               "Replier recovery attempted from a thread other than its owner" } );
        }

        // The old REP socket must release its bind before the replacement can bind the same path.
        // Move-assign the new *unbound* socket first, which closes/replaces the old one, then bind.
        try {
            zmq::socket_t socket( *m_impl->context, zmq::socket_type::rep );
            detail::Configure_New_Socket( socket );
            m_impl->socket = std::move( socket );
            m_impl->awaiting_reply = false;
            m_impl->socket.bind( m_impl->endpoint );
            return {};
        } catch( const zmq::error_t& error ) {
            m_impl->awaiting_reply = false;
            return edge::Make_Failure( detail::Map_Bind_Error( error ) );
        }
    }

    /** @copydoc Replier::Clear_Awaiting_Reply */
    void Replier::Clear_Awaiting_Reply() noexcept { m_impl->awaiting_reply = false; }

    /** @copydoc Replier::Is_Owning_Thread */
    auto Replier::Is_Owning_Thread() const noexcept -> bool {
        return m_impl && std::this_thread::get_id() == m_impl->owning_thread;
    }

    /** @copydoc Replier::Send_Reply */
    auto Replier::Send_Reply( std::string_view payload ) -> edge::Result<void, Error> {
        if( !Is_Owning_Thread() ) {
            return edge::Make_Failure(
                Error { Error_Code::PROTOCOL_STATE_ERROR, "Replier used from a thread other than its owner" } );
        }

        try {
            if( !m_impl->socket.send( zmq::message_t( payload.data(), payload.size() ), zmq::send_flags::none ) ) {
                return edge::Make_Failure( Error { Error_Code::TIMEOUT, "reply send timed out" } );
            }
        } catch( const zmq::error_t& error ) {
            return edge::Make_Failure( Error { Error_Code::SEND_FAILED, error.what() } );
        }
        return {};
    }

    /** @copydoc Replier::Receive */
    auto Replier::Receive( std::chrono::milliseconds timeout ) -> edge::Result<std::optional<Pending_Request>, Error> {
        if( std::this_thread::get_id() != m_impl->owning_thread ) {
            return edge::Make_Failure(
                Error { Error_Code::PROTOCOL_STATE_ERROR, "Replier used from a thread other than its owner" } );
        }

        if( m_impl->awaiting_reply ) {
            return edge::Make_Failure(
                Error { Error_Code::PROTOCOL_STATE_ERROR, "previous request has not been replied to" } );
        }

        zmq::message_t request;
        try {
            detail::Set_Receive_Timeout( m_impl->socket, timeout );
            const auto result = m_impl->socket.recv( request, zmq::recv_flags::none );
            if( !result ) {
                return std::optional<Pending_Request> { std::nullopt };
            }
        } catch( const zmq::error_t& error ) {
            return edge::Make_Failure( Error { Error_Code::RECEIVE_FAILED, error.what() } );
        }

        if( request.more() ) {
            const auto recreated = Recreate_Socket();
            if( !recreated.has_value() ) {
                return edge::Make_Failure( recreated.error() );
            }
            return edge::Make_Failure(
                Error { Error_Code::RECEIVE_FAILED, "malformed REQ/REP request: unexpected extra frame" } );
        }

        if( request.size() > MAX_SERIALIZED_MESSAGE_BYTES ) {
            const auto recreated = Recreate_Socket();
            if( !recreated.has_value() ) {
                return edge::Make_Failure( recreated.error() );
            }
            return edge::Make_Failure( Error { Error_Code::MESSAGE_TOO_LARGE, "request exceeds IPC message limit" } );
        }

        std::string payload( static_cast<const char*>( request.data() ), request.size() );
        m_impl->awaiting_reply = true;
        return std::optional<Pending_Request> { Pending_Request( *this, std::move( payload ) ) };
    }

    /** @copydoc Replier::Create */
    auto Replier::Create( Context& context, std::string_view endpoint ) -> edge::Result<Replier, Error> {
        return Bind( context, endpoint );
    }

}  // namespace edge::ipc
