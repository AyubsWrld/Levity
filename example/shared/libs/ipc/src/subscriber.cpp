/**
 * @file subscriber.cpp
 * @brief Implementation of subscriber.
 *
 * @details Source path: `shared/libs/ipc/src/subscriber.cpp`.
 */

#include "detail/context_impl.h"
#include "detail/endpoint.h"
#include "detail/socket.h"

#include <edge/ipc/context.h>
#include <edge/ipc/limits.h>
#include <edge/ipc/subscriber.h>

#include <stdexcept>
#include <thread>
#include <utility>

namespace edge::ipc {

    /** @brief Native SUB socket, reconnect metadata, and thread-affinity state. */
    struct Subscriber::Impl {
        /** Owned native SUB socket. */
        zmq::socket_t socket;
        /** Non-owning native context used when malformed framing requires socket recreation. */
        zmq::context_t* context;
        /** Connected endpoint retained for socket recreation. */
        std::string endpoint;
        /** Subscription prefix retained for socket recreation. */
        std::string topic_filter;
        /** Thread on which the socket was created and must be used. */
        std::thread::id owning_thread;
    };

    /** @copydoc Subscriber::Subscriber */
    Subscriber::Subscriber( std::unique_ptr<Impl> impl ) : m_impl( std::move( impl ) ) {}
    /** @copydoc Subscriber::~Subscriber */
    Subscriber::~Subscriber() = default;
    /** @copydoc Subscriber::Subscriber */
    // NOLINTNEXTLINE(performance-noexcept-move-constructor, bugprone-exception-escape)
    Subscriber::Subscriber( Subscriber&& other ) {
        if( other.m_impl && other.m_impl->owning_thread != std::this_thread::get_id() ) {
            throw std::logic_error( "Subscriber cannot be moved from a thread other than its owner" );
        }
        m_impl = std::move( other.m_impl );
    }

    /** @copydoc Subscriber::Connect */
    auto Subscriber::Connect( Context& context, std::string_view endpoint, std::string_view topic_filter )
        -> edge::Result<Subscriber, Error> {
        if( auto validation = detail::Validate_Endpoint( endpoint ); !validation.has_value() ) {
            return edge::Make_Failure( validation.error() );
        }

        try {
            zmq::socket_t socket( context.Native_Impl().context, zmq::socket_type::sub );
            detail::Configure_New_Socket( socket );
            const std::string endpoint_string( endpoint );
            const std::string topic_filter_string( topic_filter );
            auto& native_context = context.Native_Impl().context;
            socket.connect( endpoint_string );
            socket.set( zmq::sockopt::subscribe, topic_filter_string );
            return Subscriber( std::make_unique<Impl>( Impl { std::move( socket ),
                                                              &native_context,
                                                              endpoint_string,
                                                              topic_filter_string,
                                                              std::this_thread::get_id() } ) );
        } catch( const zmq::error_t& error ) {
            return edge::Make_Failure( detail::Map_Connect_Error( error ) );
        }
    }

    /** @copydoc Subscriber::Recreate_Socket */
    auto Subscriber::Recreate_Socket() -> edge::Result<void, Error> {
        try {
            zmq::socket_t socket( *m_impl->context, zmq::socket_type::sub );
            detail::Configure_New_Socket( socket );
            socket.connect( m_impl->endpoint );
            socket.set( zmq::sockopt::subscribe, m_impl->topic_filter );
            m_impl->socket = std::move( socket );
            return {};
        } catch( const zmq::error_t& error ) {
            return edge::Make_Failure( detail::Map_Connect_Error( error ) );
        }
    }

    /** @copydoc Subscriber::Receive */
    auto Subscriber::Receive( std::chrono::milliseconds timeout ) -> edge::Result<std::optional<Message>, Error> {
        if( std::this_thread::get_id() != m_impl->owning_thread ) {
            return edge::Make_Failure(
                Error { Error_Code::PROTOCOL_STATE_ERROR, "Subscriber used from a thread other than its owner" } );
        }

        const auto total_budget = detail::Normalize_Timeout( timeout );
        const auto deadline = std::chrono::steady_clock::now() + total_budget;

        zmq::message_t topic_frame;
        try {
            detail::Set_Receive_Timeout( m_impl->socket, total_budget );
            const auto result = m_impl->socket.recv( topic_frame, zmq::recv_flags::none );
            if( !result ) {
                return std::optional<Message> { std::nullopt };
            }
        } catch( const zmq::error_t& error ) {
            return edge::Make_Failure( Error { Error_Code::RECEIVE_FAILED, error.what() } );
        }

        if( !topic_frame.more() ) {
            return edge::Make_Failure(
                Error { Error_Code::RECEIVE_FAILED, "malformed PUB/SUB message: missing payload" } );
        }

        const auto now = std::chrono::steady_clock::now();
        auto payload_budget = std::chrono::milliseconds::zero();
        if( now < deadline ) {
            payload_budget = std::chrono::ceil<std::chrono::milliseconds>( deadline - now );
        }

        zmq::message_t payload_frame;
        try {
            detail::Set_Receive_Timeout( m_impl->socket, payload_budget );
            const auto result = m_impl->socket.recv( payload_frame, zmq::recv_flags::none );
            if( !result ) {
                const auto recreated = Recreate_Socket();
                if( !recreated.has_value() ) {
                    return edge::Make_Failure( recreated.error() );
                }
                return edge::Make_Failure( Error { Error_Code::RECEIVE_FAILED, "payload frame did not arrive" } );
            }
        } catch( const zmq::error_t& error ) {
            const auto recreated = Recreate_Socket();
            if( !recreated.has_value() ) {
                return edge::Make_Failure( recreated.error() );
            }
            return edge::Make_Failure( Error { Error_Code::RECEIVE_FAILED, error.what() } );
        }

        if( payload_frame.more() ) {
            const auto recreated = Recreate_Socket();
            if( !recreated.has_value() ) {
                return edge::Make_Failure( recreated.error() );
            }
            return edge::Make_Failure(
                Error { Error_Code::RECEIVE_FAILED, "malformed PUB/SUB message: unexpected extra frame" } );
        }

        if( payload_frame.size() > MAX_SERIALIZED_MESSAGE_BYTES ) {
            return edge::Make_Failure( Error { Error_Code::MESSAGE_TOO_LARGE, "payload exceeds IPC message limit" } );
        }

        Message message;
        message.topic.assign( static_cast<const char*>( topic_frame.data() ), topic_frame.size() );
        message.payload.assign( static_cast<const char*>( payload_frame.data() ), payload_frame.size() );
        return std::optional<Message> { std::move( message ) };
    }

    /** @copydoc Subscriber::Create */
    auto Subscriber::Create( Context& context, std::string_view endpoint, std::string_view topic_filter )
        -> edge::Result<Subscriber, Error> {
        return Connect( context, endpoint, topic_filter );
    }

}  // namespace edge::ipc
