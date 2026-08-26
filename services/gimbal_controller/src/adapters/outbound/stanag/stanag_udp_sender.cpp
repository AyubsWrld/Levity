/**
 * @file stanag_udp_sender.cpp
 * @brief Implementation of stanag udp sender.
 *
 * @details Source path: `services/gimbal_controller/src/adapters/outbound/stanag/stanag_udp_sender.cpp`.
 */

#include <arpa/inet.h>
#include <edge/gimbal_controller/adapters/outbound/stanag/stanag_encoder.h>
#include <edge/gimbal_controller/adapters/outbound/stanag/stanag_udp_sender.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

namespace edge::gimbal_controller::adapters::outbound::stanag {

    /** @copydoc Stanag_Udp_Sender::Stanag_Udp_Sender */
    Stanag_Udp_Sender::Stanag_Udp_Sender( std::string host, std::uint16_t port )
        : m_host( std::move( host ) ), m_port( port ) {
        const int raw_fd = ::socket( AF_INET, SOCK_DGRAM, 0 );
        if( raw_fd < 0 ) {
            throw std::runtime_error( std::string( "Stanag_Udp_Sender: socket() failed: " ) + std::strerror( errno ) );
        }
        m_socket_fd.Reset( raw_fd );

        m_destination.sin_family = AF_INET;
        m_destination.sin_port = htons( port );
        if( ::inet_pton( AF_INET, m_host.c_str(), &m_destination.sin_addr ) != 1 ) {
            throw std::runtime_error( "Stanag_Udp_Sender: invalid IPv4 host '" + m_host + "'" );
        }
    }

    /** @copydoc Stanag_Udp_Sender::Send_Datagram */
    auto Stanag_Udp_Sender::Send_Datagram( std::span<const std::byte> datagram )
        -> edge::Result<void, core::ports::outbound::Gimbal_Output_Error> {
        // BSD sockets API requires this type-pun between sockaddr_in and the generic sockaddr;
        // there is no standard-conforming alternative when calling ::sendto().
        const auto* destination_address =
            reinterpret_cast<const sockaddr*>( &m_destination );  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        const ssize_t sent_bytes = ::sendto(
            m_socket_fd.Get(), datagram.data(), datagram.size(), 0, destination_address, sizeof( m_destination ) );

        if( sent_bytes < 0 ) {
            return edge::Make_Failure( core::ports::outbound::Gimbal_Output_Error {
                std::string( "Stanag_Udp_Sender: sendto() failed: " ) + std::strerror( errno ) } );
        }
        if( static_cast<std::size_t>( sent_bytes ) != datagram.size() ) {
            return edge::Make_Failure( core::ports::outbound::Gimbal_Output_Error {
                "Stanag_Udp_Sender: sendto() reported a partial datagram send" } );
        }
        return {};
    }

    /** @copydoc Stanag_Udp_Sender::Send_Slew */
    auto Stanag_Udp_Sender::Send_Slew( const edge::telemetry::v1::Target& target )
        -> edge::Result<void, core::ports::outbound::Gimbal_Output_Error> {
        const std::uint32_t sequence = m_next_sequence++;

        const auto steering_command = Encode_Steering_Command_Message(
            target.latitude_deg(), target.longitude_deg(), target.altitude_m(), sequence );
        if( auto result = Send_Datagram( steering_command ); !result.has_value() ) {
            return result;
        }

        return Send_Datagram( Encode_Steering_Mode_Message( sequence ) );
    }

}  // namespace edge::gimbal_controller::adapters::outbound::stanag
