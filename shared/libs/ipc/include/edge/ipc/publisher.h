/**
 * @file publisher.h
 * @brief Public PUB socket wrapper for topic-framed local telemetry publication.
 */
#pragma once

#include <edge/ipc/error.h>
#include <edge/result.h>

#include <memory>
#include <string_view>

namespace edge::ipc {

    class Context;

    /**
     * @brief Move-only PUB endpoint with transport details hidden behind a private implementation.
     *
     * @details The wrapper owns one native socket and enforces message-size/framing rules. A Publisher
     * must be created, used, and destroyed on the same owning thread. The referenced Context must
     * outlive the Publisher.
     */
    class Publisher final {
    public:
        /**
         * @brief Binds a PUB socket to @p endpoint.
         * @param context Shared process IPC context that outlives the returned Publisher.
         * @param endpoint ZeroMQ endpoint, normally an `ipc://` path.
         * @return Move-only Publisher on success, otherwise a transport/endpoint Error.
         *
         * @details For `ipc://` endpoints the parent directory is created when missing with owner-only
         * permissions. Existing socket files are never unlinked automatically. Production deployments
         * whose service containers run under different UIDs/groups should pre-create the shared IPC
         * directory with deployment-appropriate permissions; the library never chmods an existing directory.
         */
        [[nodiscard]] static auto Bind( Context& context, std::string_view endpoint ) -> edge::Result<Publisher, Error>;

        /**
         * @brief Compatibility alias for Bind().
         * @deprecated Prefer Bind() because it states the socket role explicitly.
         */
        [[nodiscard]] static auto Create( Context& context, std::string_view endpoint )
            -> edge::Result<Publisher, Error>;

        /** @brief Closes the owned native PUB socket on the owning thread. */
        ~Publisher();

        Publisher( const Publisher& ) = delete;
        auto operator=( const Publisher& ) -> Publisher& = delete;
        /**
         * @brief Transfers the native PUB wrapper on its owning thread.
         * @param other Source wrapper.
         * @throws std::logic_error if the move is attempted from a thread other than the socket owner.
         * @details Intentionally not `noexcept`: thread-affinity misuse must propagate as a
         * catchable exception rather than terminate the process.
         */
        // NOLINTNEXTLINE(performance-noexcept-move-constructor, bugprone-exception-escape)
        Publisher( Publisher&& other );
        // Move construction supports factory return values; move assignment is forbidden because it
        // could close/replace a thread-affine native socket on the wrong thread.
        auto operator=( Publisher&& ) noexcept -> Publisher& = delete;

        /**
         * @brief Publishes one logical topic/payload message.
         * @param topic Topic frame used by subscribers for prefix filtering.
         * @param payload Serialized domain payload frame.
         * @return Success or an Error such as MESSAGE_TOO_LARGE/SEND_FAILED. Both topic and payload
         * frames are bounded by MAX_SERIALIZED_MESSAGE_BYTES.
         *
         * @note PUB/SUB does not guarantee that newly connected subscribers receive the first
         * publication; callers/tests must account for subscription propagation/slow joiners. If a
         * multipart send fails after it begins, the private PUB socket is recreated before another
         * publication is accepted so incomplete framing cannot contaminate the next message.
         */
        [[nodiscard]] auto Publish( std::string_view topic, std::string_view payload ) -> edge::Result<void, Error>;

    private:
        /** Opaque native PUB socket implementation. */
        struct Impl;
        /** @brief Constructs from a successfully initialized private implementation. */
        explicit Publisher( std::unique_ptr<Impl> impl );
        /** @brief Rebinds a fresh PUB socket after an interrupted multipart send. */
        [[nodiscard]] auto Recreate_Socket() -> edge::Result<void, Error>;

        /** Sole owner of the transport-specific socket state. */
        std::unique_ptr<Impl> m_impl;
    };

}  // namespace edge::ipc
