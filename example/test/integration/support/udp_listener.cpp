/**
 * @file udp_listener.cpp
 * @brief Test implementation for udp listener behavior and regression coverage.
 *
 * @details Source path: `test/integration/support/udp_listener.cpp`.
 */

#include "udp_listener.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>

#include <array>
#include <cerrno>
#include <limits>
#include <stdexcept>

namespace edge::integration_test::support {
    namespace {

        /** @brief Converts a chrono timeout to poll()'s finite signed-int millisecond range. */
        [[nodiscard]] auto Poll_Timeout( std::chrono::milliseconds timeout ) noexcept -> int {
            if( timeout <= std::chrono::milliseconds::zero() ) {
                return 0;
            }
            constexpr auto MAX_POLL_TIMEOUT = std::chrono::milliseconds { std::numeric_limits<int>::max() };
            const auto normalized = timeout > MAX_POLL_TIMEOUT ? MAX_POLL_TIMEOUT : timeout;
            return static_cast<int>( normalized.count() );
        }

        /** UDP receive buffer size, comfortably larger than any STANAG datagram exchanged in tests. */
        constexpr std::size_t DATAGRAM_BUFFER_SIZE = 4096;

    }  // namespace

    /** @copydoc Udp_Listener::Udp_Listener */
    Udp_Listener::Udp_Listener() : m_fd( ::socket( AF_INET, SOCK_DGRAM, 0 ) ) {
        if( !m_fd ) {
            throw std::runtime_error( "Udp_Listener: socket() failed" );
        }

        sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
        address.sin_port = 0;

        if( ::bind( m_fd.Get(),
                    reinterpret_cast<sockaddr*>( &address ),
                    sizeof( address ) ) !=  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            0 ) {
            throw std::runtime_error( "Udp_Listener: bind() failed" );
        }

        sockaddr_in bound_address {};
        socklen_t bound_address_length = sizeof( bound_address );
        if( ::getsockname( m_fd.Get(),
                           reinterpret_cast<sockaddr*>( &bound_address ),
                           &bound_address_length ) !=  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            0 ) {
            throw std::runtime_error( "Udp_Listener: getsockname() failed" );
        }

        m_port = ntohs( bound_address.sin_port );
    }

    /** @copydoc Udp_Listener::Port */
    auto Udp_Listener::Port() const noexcept -> std::uint16_t { return m_port; }

    /** @copydoc Udp_Listener::Receive_Datagram */
    auto Udp_Listener::Receive_Datagram( std::chrono::milliseconds timeout ) -> std::optional<std::vector<std::byte>> {
        auto remaining = timeout;
        const auto started_waiting_at = std::chrono::steady_clock::now();

        for( ;; ) {
            pollfd poll_target { .fd = m_fd.Get(), .events = POLLIN, .revents = 0 };
            const int poll_result = ::poll( &poll_target, 1, Poll_Timeout( remaining ) );

            if( poll_result < 0 ) {
                if( errno == EINTR ) {
                    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - started_waiting_at );
                    remaining = timeout - elapsed;
                    if( remaining.count() <= 0 ) {
                        return std::nullopt;
                    }
                    continue;
                }
                return std::nullopt;
            }
            if( poll_result == 0 ) {
                return std::nullopt;
            }

            std::array<std::byte, DATAGRAM_BUFFER_SIZE> buffer {};
            const auto bytes_received = ::recvfrom( m_fd.Get(), buffer.data(), buffer.size(), 0, nullptr, nullptr );
            if( bytes_received < 0 ) {
                return std::nullopt;
            }

            return std::vector<std::byte>( buffer.begin(), buffer.begin() + bytes_received );
        }
    }

    /** @copydoc Udp_Listener::Drain_Pending */
    void Udp_Listener::Drain_Pending() {
        std::array<std::byte, DATAGRAM_BUFFER_SIZE> buffer {};
        while( ::recvfrom( m_fd.Get(), buffer.data(), buffer.size(), MSG_DONTWAIT, nullptr, nullptr ) >= 0 ) {
        }
    }

}  // namespace edge::integration_test::support
