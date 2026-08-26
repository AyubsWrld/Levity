/**
 * @file udp_listener.h
 * @brief RAII loopback UDP listener used to observe gimbal hardware-adapter datagrams in tests.
 */
#pragma once

#include <edge/platform/posix/unique_fd.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace edge::integration_test::support {

    /**
     * @brief Binds an ephemeral loopback UDP port and captures datagrams with bounded waits.
     *
     * @details The listener itself owns the port allocation, eliminating the bind race that would
     * occur if tests probed a free UDP port and closed it before the service started sending.
     */
    class Udp_Listener final {
    public:
        /**
         * @brief Creates/binds a loopback UDP socket on an ephemeral port.
         * @throws std::runtime_error if socket creation, bind, or getsockname fails.
         */
        Udp_Listener();
        ~Udp_Listener() = default;

        Udp_Listener( const Udp_Listener& ) = delete;
        auto operator=( const Udp_Listener& ) -> Udp_Listener& = delete;
        Udp_Listener( Udp_Listener&& ) noexcept = default;
        auto operator=( Udp_Listener&& ) noexcept -> Udp_Listener& = default;

        /** @brief Returns the bound UDP port advertised to the gimbal service. */
        [[nodiscard]] auto Port() const noexcept -> std::uint16_t;

        /**
         * @brief Waits up to @p timeout for one UDP datagram.
         * @param timeout Requested wait; non-positive values poll immediately and oversized values
         * are clamped to the largest finite timeout accepted by poll().
         * @return Owned datagram bytes, or std::nullopt when the timeout expires or a poll/receive
         * error prevents a datagram from being returned. This test helper intentionally collapses
         * those failures because callers diagnose service behavior separately through process logs.
         */
        [[nodiscard]] auto Receive_Datagram( std::chrono::milliseconds timeout )
            -> std::optional<std::vector<std::byte>>;

        /** @brief Drains all currently queued datagrams without blocking. */
        void Drain_Pending();

    private:
        /** Owned bound UDP socket. */
        edge::platform::posix::Unique_Fd m_fd;
        /** Ephemeral port assigned by the kernel. */
        std::uint16_t m_port = 0;
    };

}  // namespace edge::integration_test::support
