/**
 * @file subscriber.h
 * @brief Public SUB socket wrapper for receiving topic-framed local telemetry messages.
 */
#pragma once

#include <edge/ipc/error.h>
#include <edge/ipc/message.h>
#include <edge/result.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string_view>

namespace edge::ipc {

    class Context;

    /**
     * @brief Move-only SUB endpoint with bounded receive semantics.
     *
     * @details The native ZeroMQ socket is hidden from consumers and is thread-affine: create/use/
     * destroy a Subscriber on one owning thread. Context may be shared but must outlive the wrapper.
     * The wire contract is exactly two frames (topic, payload); malformed multipart input is rejected
     * and the private socket is recreated so later receives start from a clean framing state.
     */
    class Subscriber final {
    public:
        /**
         * @brief Connects a SUB socket and applies a topic-prefix subscription filter.
         * @param context Shared IPC context that outlives the returned Subscriber.
         * @param endpoint Publisher endpoint to connect to.
         * @param topic_filter ZeroMQ subscription prefix; empty subscribes to all topics.
         * @return Subscriber on success or a transport/configuration Error.
         */
        [[nodiscard]] static auto Connect( Context& context, std::string_view endpoint, std::string_view topic_filter )
            -> edge::Result<Subscriber, Error>;

        /** @brief Compatibility alias for Connect(). @deprecated Prefer Connect(). */
        [[nodiscard]] static auto Create( Context& context, std::string_view endpoint, std::string_view topic_filter )
            -> edge::Result<Subscriber, Error>;

        /** @brief Closes the native SUB socket on destruction. */
        ~Subscriber();

        Subscriber( const Subscriber& ) = delete;
        auto operator=( const Subscriber& ) -> Subscriber& = delete;
        /**
         * @brief Transfers the native SUB wrapper on its owning thread.
         * @param other Source wrapper.
         * @throws std::logic_error if the move is attempted from a thread other than the socket owner.
         * @details Intentionally not `noexcept`: thread-affinity misuse must propagate as a
         * catchable exception rather than terminate the process.
         */
        // NOLINTNEXTLINE(performance-noexcept-move-constructor, bugprone-exception-escape)
        Subscriber( Subscriber&& other );
        // Move construction supports factory return values; move assignment is forbidden because it
        // could close/replace a thread-affine native socket on the wrong thread.
        auto operator=( Subscriber&& ) noexcept -> Subscriber& = delete;

        /**
         * @brief Waits up to @p timeout for one complete topic/payload message.
         * @param timeout Maximum total wait for the complete topic/payload message. Non-positive
         * values perform non-blocking receives; oversized values are clamped to a finite native maximum.
         * @return `Message` when received, `std::nullopt` when the timeout elapsed normally, or Error
         * for transport/framing/message-size failures.
         */
        [[nodiscard]] auto Receive( std::chrono::milliseconds timeout ) -> edge::Result<std::optional<Message>, Error>;

    private:
        /** Opaque native SUB socket implementation. */
        struct Impl;
        explicit Subscriber( std::unique_ptr<Impl> impl );
        /** @brief Recreates/reconnects the private SUB socket after malformed/incomplete multipart input. */
        [[nodiscard]] auto Recreate_Socket() -> edge::Result<void, Error>;
        /** Sole owner of private transport state. */
        std::unique_ptr<Impl> m_impl;
    };

}  // namespace edge::ipc
