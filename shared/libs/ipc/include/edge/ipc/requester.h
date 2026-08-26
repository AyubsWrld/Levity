/**
 * @file requester.h
 * @brief Public synchronous REQ wrapper for acknowledged point-to-point commands.
 */
#pragma once

#include <edge/ipc/error.h>
#include <edge/result.h>

#include <chrono>
#include <memory>
#include <string>
#include <string_view>

namespace edge::ipc {

    class Context;

    /**
     * @brief Move-only ZeroMQ REQ wrapper enforcing one-request/one-reply sequencing.
     *
     * @details The wrapper is thread-affine and must be used only on its owning thread. After an
     * ambiguous timeout the native REQ socket is recreated so a later *distinct* Request() can start
     * from a valid protocol state. The timed-out request is never transparently retried. Requests
     * and replies are each exactly one frame; a multipart reply is rejected and the socket is
     * recreated before another request is allowed.
     */
    class Requester final {
    public:
        /**
         * @brief Connects a REQ socket to @p endpoint.
         * @param context Shared IPC context that outlives the returned Requester.
         * @param endpoint REP peer endpoint.
         * @return Requester on success or CONNECT_FAILED/INVALID_ENDPOINT/etc. on failure.
         */
        [[nodiscard]] static auto Connect( Context& context, std::string_view endpoint )
            -> edge::Result<Requester, Error>;

        /** @brief Compatibility alias for Connect(). @deprecated Prefer Connect(). */
        [[nodiscard]] static auto Create( Context& context, std::string_view endpoint )
            -> edge::Result<Requester, Error>;

        /** @brief Closes the native REQ socket. */
        ~Requester();

        Requester( const Requester& ) = delete;
        auto operator=( const Requester& ) -> Requester& = delete;
        /**
         * @brief Transfers the native REQ wrapper on its owning thread.
         * @param other Source wrapper.
         * @throws std::logic_error if the move is attempted from a thread other than the socket owner.
         * @details Intentionally not `noexcept`: thread-affinity misuse must propagate as a
         * catchable exception rather than terminate the process.
         */
        // NOLINTNEXTLINE(performance-noexcept-move-constructor, bugprone-exception-escape)
        Requester( Requester&& other );
        // Move construction supports factory return values; move assignment is forbidden because it
        // could close/replace a thread-affine native socket on the wrong thread.
        auto operator=( Requester&& ) noexcept -> Requester& = delete;

        /**
         * @brief Sends one request and waits for exactly one reply.
         * @param payload Serialized request payload.
         * @param timeout Maximum total budget for sending the request and receiving its reply. Non-positive
         * values perform non-blocking send/receive attempts; oversized values are clamped to the
         * largest finite timeout representable by the native transport.
         * @return Owned reply payload or a transport/protocol Error.
         *
         * @warning A `TIMEOUT` returned after the request was accepted for sending is an ambiguous
         * outcome: the REP peer may already have acted and only the reply may have been lost/delayed.
         * This function never retries the request. The supplied timeout is one end-to-end budget,
         * not an independent send timeout plus a second receive timeout.
         */
        [[nodiscard]] auto Request( std::string_view payload, std::chrono::milliseconds timeout )
            -> edge::Result<std::string, Error>;

    private:
        /** Opaque REQ socket implementation including the endpoint needed for state recovery. */
        struct Impl;
        explicit Requester( std::unique_ptr<Impl> impl );

        /** @brief Recreates the native REQ socket after a failed/ambiguous protocol cycle. */
        [[nodiscard]] auto Recreate_Socket() -> edge::Result<void, Error>;

        /** Sole owner of native request transport state. */
        std::unique_ptr<Impl> m_impl;
    };

}  // namespace edge::ipc
