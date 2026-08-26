/**
 * @file replier.h
 * @brief Public REP wrapper and RAII pending-request token for command servers.
 */
#pragma once

#include <edge/ipc/error.h>
#include <edge/result.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace edge::ipc {

    class Context;
    class Replier;

    /**
     * @brief Move-only token representing one received REP request awaiting its required reply.
     *
     * @details ZeroMQ REP requires a reply before the socket may receive the next request. This token
     * makes that protocol state explicit. Reply() may be called once. Destroying/abandoning a token
     * without replying does not fabricate a response or clear the owner state; a subsequent Receive()
     * reports PROTOCOL_STATE_ERROR. Production server code is therefore expected to reply to every
     * successfully received request, including malformed/application-rejected requests.
     */
    class Pending_Request final {
    public:
        /** @brief Releases the local token; it does not synthesize or clear a missing REP reply. */
        ~Pending_Request();

        Pending_Request( const Pending_Request& ) = delete;
        auto operator=( const Pending_Request& ) -> Pending_Request& = delete;
        Pending_Request( Pending_Request&& other ) noexcept;
        auto operator=( Pending_Request&& other ) noexcept -> Pending_Request& = delete;

        /** @brief Returns a non-owning view of the received request payload stored by this token. */
        [[nodiscard]] auto Payload() const noexcept -> std::string_view;

        /**
         * @brief Sends the mandatory REP reply for this request.
         * @param payload Serialized reply payload.
         * @return Success or a send/protocol/size Error.
         * @note A MESSAGE_TOO_LARGE rejection occurs before any transport send and leaves the token
         * pending so the caller may provide a smaller reply. Once a transport send is attempted, the
         * request cycle is consumed and must not be retried through this token.
         */
        [[nodiscard]] auto Reply( std::string_view payload ) -> edge::Result<void, Error>;

    private:
        friend class Replier;
        /** @brief Created only by Replier::Receive() for a successfully received request. */
        Pending_Request( Replier& owner, std::string payload );

        /** Non-owning owner pointer; nulled/moved as ownership of the pending cycle changes. */
        Replier* m_owner;
        /** Owned request bytes returned through Payload(). */
        std::string m_payload;
        /** Tracks whether the required response has already been emitted. */
        bool m_replied = false;
    };

    /**
     * @brief Move-only ZeroMQ REP server wrapper with explicit request-cycle ownership.
     *
     * @details The wrapper is thread-affine and should be created/used/destroyed on one worker. A
     * successful Receive() returns Pending_Request, which must successfully Reply() before another
     * request cycle can proceed. Destroying an unreplied token intentionally leaves the wrapper in a
     * protocol-state error rather than fabricating a reply. The request wire contract is exactly one
     * frame; malformed multipart requests are rejected by recreating the private REP socket.
     * Bind-side filesystem preparation is handled internally.
     */
    class Replier final {
    public:
        /**
         * @brief Binds a REP socket to @p endpoint.
         * @param context Shared IPC context that outlives the returned Replier.
         * @param endpoint Command endpoint to bind.
         * @return Replier or endpoint/bind error.
         * @note Missing IPC parent directories are created owner-only. Deployments using different
         * service UIDs/groups should pre-create the shared directory with appropriate permissions.
         */
        [[nodiscard]] static auto Bind( Context& context, std::string_view endpoint ) -> edge::Result<Replier, Error>;

        /** @brief Compatibility alias for Bind(). @deprecated Prefer Bind(). */
        [[nodiscard]] static auto Create( Context& context, std::string_view endpoint ) -> edge::Result<Replier, Error>;

        /** @brief Closes the native REP socket. */
        ~Replier();

        Replier( const Replier& ) = delete;
        auto operator=( const Replier& ) -> Replier& = delete;
        /**
         * @brief Transfers ownership of the native REP wrapper on its owning thread.
         * @param other Source wrapper.
         * @throws std::logic_error if called from a non-owning thread or while a Pending_Request is live.
         * @details The runtime check prevents a live request token from retaining a pointer to a
         * moved-from wrapper and prevents native socket destruction from migrating across threads.
         * Intentionally not `noexcept` so these violations propagate as catchable exceptions
         * rather than terminating the process.
         */
        // NOLINTNEXTLINE(performance-noexcept-move-constructor, bugprone-exception-escape)
        Replier( Replier&& other );
        auto operator=( Replier&& ) noexcept -> Replier& = delete;

        /**
         * @brief Waits for the next request while no previous request is pending.
         * @param timeout Maximum wait used by stoppable server loops. Non-positive values perform a non-blocking
         * receive; oversized values are clamped to a finite native maximum.
         * @return Pending_Request when a request arrives, std::nullopt on normal timeout, or Error.
         */
        [[nodiscard]] auto Receive( std::chrono::milliseconds timeout )
            -> edge::Result<std::optional<Pending_Request>, Error>;

    private:
        friend class Pending_Request;
        /** Opaque native REP socket implementation. */
        struct Impl;
        explicit Replier( std::unique_ptr<Impl> impl );

        /** @brief Rebinds a fresh REP socket when an incomplete cycle invalidates native state. */
        [[nodiscard]] auto Recreate_Socket() -> edge::Result<void, Error>;
        /** @brief Returns true when called on the native REP socket's owning thread. */
        [[nodiscard]] auto Is_Owning_Thread() const noexcept -> bool;
        /** @brief Sends the reply associated with the currently pending request. */
        [[nodiscard]] auto Send_Reply( std::string_view payload ) -> edge::Result<void, Error>;
        /** @brief Marks the request cycle complete without performing further transport work. */
        void Clear_Awaiting_Reply() noexcept;

        /** Sole owner of native REP state. */
        std::unique_ptr<Impl> m_impl;
    };

}  // namespace edge::ipc
