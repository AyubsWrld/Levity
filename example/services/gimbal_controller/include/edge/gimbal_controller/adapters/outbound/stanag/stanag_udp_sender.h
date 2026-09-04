/**
 * @file stanag_udp_sender.h
 * @brief UDP/STANAG implementation of the Gimbal Controller outbound hardware port.
 */
#pragma once

#include <edge/gimbal_controller/core/ports/outbound/gimbal_output_port.h>
#include <edge/platform/posix/unique_fd.h>
#include <edge/result.h>
#include <netinet/in.h>

#include <cstdint>
#include <span>
#include <string>

namespace edge::gimbal_controller::adapters::outbound::stanag {

    /**
     * @brief Translates domain targets into dummy STANAG steering datagrams and sends them over UDP.
     *
     * @details Construction performs startup/configuration work (destination parsing and socket
     * creation) and throws on unrecoverable startup failure. Runtime send failures are returned as
     * Gimbal_Output_Error through edge::Result. The adapter owns its POSIX socket via Unique_Fd.
     * The adapter is intentionally not internally synchronized; current runtime composition drives
     * Gimbal_Service/this output port serially from the single inbound command worker.
     */
    class Stanag_Udp_Sender final : public core::ports::outbound::Gimbal_Output_Port {
    public:
        /**
         * @brief Creates the UDP sender for a fixed destination.
         * @param host Destination IPv4 address literal accepted by inet_pton().
         * @param port Destination UDP port.
         * @throws std::runtime_error for invalid destination configuration or socket creation failure.
         */
        Stanag_Udp_Sender( std::string host, std::uint16_t port );
        ~Stanag_Udp_Sender() override = default;

        Stanag_Udp_Sender( const Stanag_Udp_Sender& ) = delete;
        auto operator=( const Stanag_Udp_Sender& ) -> Stanag_Udp_Sender& = delete;
        Stanag_Udp_Sender( Stanag_Udp_Sender&& ) noexcept = default;
        auto operator=( Stanag_Udp_Sender&& ) noexcept -> Stanag_Udp_Sender& = default;

        /** @copydoc core::ports::outbound::Gimbal_Output_Port::Send_Slew() */
        [[nodiscard]] auto Send_Slew( const edge::telemetry::v1::Target& target )
            -> edge::Result<void, core::ports::outbound::Gimbal_Output_Error> override;

        /** @brief Returns the configured destination host string. */
        [[nodiscard]] auto Host() const noexcept -> const std::string& { return m_host; }
        /** @brief Returns the configured destination UDP port. */
        [[nodiscard]] auto Port() const noexcept -> std::uint16_t { return m_port; }

    private:
        /**
         * @brief Sends one already-encoded datagram to the configured destination.
         * @param datagram Wire bytes whose lifetime only needs to span this call.
         * @return Success or a transport diagnostic; partial/failed sends are errors.
         */
        [[nodiscard]] auto Send_Datagram( std::span<const std::byte> datagram )
            -> edge::Result<void, core::ports::outbound::Gimbal_Output_Error>;

        /** Owned UDP socket descriptor. */
        edge::platform::posix::Unique_Fd m_socket_fd;
        /** Parsed sockaddr used by each sendto() operation. */
        sockaddr_in m_destination {};
        /** Original configured host retained for diagnostics/introspection. */
        std::string m_host;
        /** Original configured UDP port. */
        std::uint16_t m_port;
        /** Next adapter-local sequence number written into outbound steering messages. */
        std::uint32_t m_next_sequence = 1;
    };

}  // namespace edge::gimbal_controller::adapters::outbound::stanag
