/**
 * @file publisher.cpp
 * @brief Implementation of publisher.
 *
 * @details Source path: `shared/libs/ipc/src/publisher.cpp`.
 */

#include "detail/context_impl.h"
#include "detail/endpoint.h"
#include "detail/socket.h"

#include <edge/ipc/context.h>
#include <edge/ipc/limits.h>
#include <edge/ipc/publisher.h>

#include <stdexcept>
#include <thread>
#include <utility>

namespace edge::ipc {

    /** @brief Native PUB socket plus bind metadata and thread-affinity state. */
    struct Publisher::Impl {
        /** Owned native PUB socket. */
        zmq::socket_t socket;
        /** Non-owning native context used to recreate an interrupted multipart send. */
        zmq::context_t* context;
        /** Bound endpoint retained for socket-state recovery. */
        std::string endpoint;
        /** Thread on which the socket was created and must be used. */
        std::thread::id owning_thread;
    };

    /** @copydoc Publisher::Publisher */
    Publisher::Publisher( std::unique_ptr<Impl> impl ) : m_impl( std::move( impl ) ) {}
    /** @copydoc Publisher::~Publisher */
    Publisher::~Publisher() = default;
    /** @copydoc Publisher::Publisher */
    // NOLINTNEXTLINE(performance-noexcept-move-constructor, bugprone-exception-escape)
    Publisher::Publisher( Publisher&& other ) {
        if( other.m_impl && other.m_impl->owning_thread != std::this_thread::get_id() ) {
            throw std::logic_error( "Publisher cannot be moved from a thread other than its owner" );
        }
        m_impl = std::move( other.m_impl );
    }

    /** @copydoc Publisher::Bind */
    auto Publisher::Bind( Context& context, std::string_view endpoint ) -> edge::Result<Publisher, Error> {
        if( auto prepared = detail::Prepare_Bind_Endpoint( endpoint ); !prepared.has_value() ) {
            return edge::Make_Failure( prepared.error() );
        }

        auto& native_context = context.Native_Impl().context;
        try {
            zmq::socket_t socket( native_context, zmq::socket_type::pub );
            detail::Configure_New_Socket( socket );
            const std::string endpoint_string( endpoint );
            socket.bind( endpoint_string );
            return Publisher( std::make_unique<Impl>(
                Impl { std::move( socket ), &native_context, endpoint_string, std::this_thread::get_id() } ) );
        } catch( const zmq::error_t& error ) {
            return edge::Make_Failure( detail::Map_Bind_Error( error ) );
        }
    }

    /** @copydoc Publisher::Recreate_Socket */
    auto Publisher::Recreate_Socket() -> edge::Result<void, Error> {
        try {
            zmq::socket_t socket( *m_impl->context, zmq::socket_type::pub );
            detail::Configure_New_Socket( socket );
            // Release the current bind before the replacement binds the same ipc:// path.
            m_impl->socket = std::move( socket );
            m_impl->socket.bind( m_impl->endpoint );
            return {};
        } catch( const zmq::error_t& error ) {
            return edge::Make_Failure( detail::Map_Bind_Error( error ) );
        }
    }

    /** @copydoc Publisher::Publish */
    auto Publisher::Publish( std::string_view topic, std::string_view payload ) -> edge::Result<void, Error> {
        if( std::this_thread::get_id() != m_impl->owning_thread ) {
            return edge::Make_Failure(
                Error { Error_Code::PROTOCOL_STATE_ERROR, "Publisher used from a thread other than its owner" } );
        }

        if( topic.size() > MAX_SERIALIZED_MESSAGE_BYTES ) {
            return edge::Make_Failure( Error { Error_Code::MESSAGE_TOO_LARGE, "topic exceeds IPC message limit" } );
        }
        if( payload.size() > MAX_SERIALIZED_MESSAGE_BYTES ) {
            return edge::Make_Failure( Error { Error_Code::MESSAGE_TOO_LARGE, "payload exceeds IPC message limit" } );
        }

        try {
            if( !m_impl->socket.send( zmq::message_t( topic.data(), topic.size() ), zmq::send_flags::sndmore ) ) {
                const auto recreated = Recreate_Socket();
                if( !recreated.has_value() ) {
                    return edge::Make_Failure( recreated.error() );
                }
                return edge::Make_Failure( Error { Error_Code::SEND_FAILED, "topic frame send did not complete" } );
            }
            if( !m_impl->socket.send( zmq::message_t( payload.data(), payload.size() ), zmq::send_flags::none ) ) {
                const auto recreated = Recreate_Socket();
                if( !recreated.has_value() ) {
                    return edge::Make_Failure( recreated.error() );
                }
                return edge::Make_Failure( Error { Error_Code::SEND_FAILED, "payload frame send did not complete" } );
            }
        } catch( const zmq::error_t& error ) {
            const auto recreated = Recreate_Socket();
            if( !recreated.has_value() ) {
                return edge::Make_Failure( recreated.error() );
            }
            return edge::Make_Failure( Error { Error_Code::SEND_FAILED, error.what() } );
        }
        return {};
    }

    /** @copydoc Publisher::Create */
    auto Publisher::Create( Context& context, std::string_view endpoint ) -> edge::Result<Publisher, Error> {
        return Bind( context, endpoint );
    }

}  // namespace edge::ipc
