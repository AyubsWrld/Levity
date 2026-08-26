/**
 * @file context.h
 * @brief Public RAII owner for the shared ZeroMQ context used by edge IPC adapters.
 */
#pragma once

#include <memory>

namespace edge::ipc {

    /**
     * @brief Process-local IPC context shared by Publisher/Subscriber/Requester/Replier instances.
     *
     * @details The concrete ZeroMQ context is hidden behind a private implementation so consumers of
     * `<edge/ipc/...>` never include or depend on `zmq.hpp`. A Context may be shared across threads,
     * but each socket wrapper created from it remains thread-affine to the thread that uses it.
     *
     * The context must outlive every IPC socket wrapper created from it. Runtime composition roots
     * should therefore declare Context before adapters whose constructors receive it.
     */
    class Context final {
    public:
        /** @brief Creates a live process-local transport context. */
        Context();
        /** @brief Destroys the native context after all dependent socket wrappers are gone. */
        ~Context();

        Context( const Context& ) = delete;
        auto operator=( const Context& ) -> Context& = delete;

        // A Context is intentionally immovable: socket wrappers retain non-owning pointers to its
        // hidden native implementation, so moving/replacing a live context would make their lifetime
        // contract easy to violate.
        Context( Context&& ) = delete;
        auto operator=( Context&& ) -> Context& = delete;

    private:
        friend class Publisher;
        friend class Subscriber;
        friend class Requester;
        friend class Replier;

        /** Opaque native-context implementation visible only to IPC wrapper friends. */
        struct Impl;
        /** @brief Returns the transport implementation used to construct private socket wrappers. */
        [[nodiscard]] auto Native_Impl() noexcept -> Impl&;

        /** Sole owner of the hidden native context. */
        std::unique_ptr<Impl> m_impl;
    };

    /**
     * @brief Backward-compatible alias for the original Tier-1 API name.
     * @deprecated Prefer edge::ipc::Context in new code.
     */
    using Ipc_Context = Context;

}  // namespace edge::ipc
