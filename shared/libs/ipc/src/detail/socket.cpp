/**
 * @file socket.cpp
 * @brief Private implementation of socket; not part of the public module API.
 *
 * @details Source path: `shared/libs/ipc/src/detail/socket.cpp`.
 */

#include "detail/socket.h"

#include <cerrno>
#include <cstdint>
#include <limits>

namespace edge::ipc::detail {

    /** @brief Applies common linger, timeout, and maximum-message-size options to a native socket. */
    void Configure_New_Socket( zmq::socket_t& socket ) {
        socket.set( zmq::sockopt::linger, 0 );
        socket.set( zmq::sockopt::sndtimeo, static_cast<int>( DEFAULT_SOCKET_TIMEOUT.count() ) );
        socket.set( zmq::sockopt::rcvtimeo, static_cast<int>( DEFAULT_SOCKET_TIMEOUT.count() ) );
        socket.set( zmq::sockopt::maxmsgsize, static_cast<std::int64_t>( MAX_SERIALIZED_MESSAGE_BYTES ) );
    }

    /** @copydoc Normalize_Timeout() */
    auto Normalize_Timeout( std::chrono::milliseconds timeout ) noexcept -> std::chrono::milliseconds {
        if( timeout <= std::chrono::milliseconds::zero() ) {
            return std::chrono::milliseconds::zero();
        }

        constexpr auto MAX_NATIVE_TIMEOUT = std::chrono::milliseconds { std::numeric_limits<int>::max() };
        return timeout > MAX_NATIVE_TIMEOUT ? MAX_NATIVE_TIMEOUT : timeout;
    }

    /** @copydoc Set_Send_Timeout() */
    void Set_Send_Timeout( zmq::socket_t& socket, std::chrono::milliseconds timeout ) {
        const auto normalized = Normalize_Timeout( timeout );
        socket.set( zmq::sockopt::sndtimeo, static_cast<int>( normalized.count() ) );
    }

    /** @copydoc Set_Receive_Timeout() */
    void Set_Receive_Timeout( zmq::socket_t& socket, std::chrono::milliseconds timeout ) {
        const auto normalized = Normalize_Timeout( timeout );
        socket.set( zmq::sockopt::rcvtimeo, static_cast<int>( normalized.count() ) );
    }

    /** @copydoc Map_Bind_Error() */
    auto Map_Bind_Error( const zmq::error_t& error ) -> Error {
        switch( error.num() ) {
            case EINVAL:
            case EPROTONOSUPPORT:
                return Error { Error_Code::INVALID_ENDPOINT, error.what() };
            default:
                return Error { Error_Code::BIND_FAILED, error.what() };
        }
    }

    /** @copydoc Map_Connect_Error() */
    auto Map_Connect_Error( const zmq::error_t& error ) -> Error {
        switch( error.num() ) {
            case EINVAL:
            case EPROTONOSUPPORT:
                return Error { Error_Code::INVALID_ENDPOINT, error.what() };
            default:
                return Error { Error_Code::CONNECT_FAILED, error.what() };
        }
    }

}  // namespace edge::ipc::detail
