/**
 * @file api_gateway.h
 * @brief Runtime composition and lifecycle root for the API Gateway executable.
 */
#pragma once

#include <edge/api_gateway/runtime/config.h>

#include <cstdint>
#include <memory>

namespace edge::platform::logging {
    class Logger;
}  // namespace edge::platform::logging

namespace edge::api_gateway::runtime {

    /**
     * @brief Owns and wires the complete API Gateway process dependency graph.
     *
     * @details Runtime is the only production layer that knows concrete core services and concrete
     * adapters simultaneously. Core never depends on adapters, and adapters target only core ports.
     * This class therefore acts as the explicit composition root and controls deterministic startup
     * and shutdown ordering.
     *
     * The Logger is non-owning and must outlive this object. All other runtime components are owned
     * by the hidden Impl in dependency-safe declaration order.
     */
    class Api_Gateway final {
    public:
        /**
         * @brief Constructs and starts all required API Gateway components.
         * @param config Fully resolved startup configuration.
         * @param logger Process logger that must outlive the gateway.
         * @throws std::exception Failures from configuration-selected adapter/resource initialization.
         */
        Api_Gateway( const Api_Gateway_Config& config, edge::platform::logging::Logger& logger );

        /** @brief Stops owned workers/listeners and releases runtime resources. */
        ~Api_Gateway();

        Api_Gateway( const Api_Gateway& ) = delete;
        auto operator=( const Api_Gateway& ) -> Api_Gateway& = delete;
        Api_Gateway( Api_Gateway&& ) = delete;
        auto operator=( Api_Gateway&& ) -> Api_Gateway& = delete;

        /**
         * @brief Returns the actual TCP port bound by the REST adapter.
         * @return Bound port, useful when tests request an ephemeral port.
         */
        [[nodiscard]] auto Bound_Http_Port() const noexcept -> std::uint16_t;

        /**
         * @brief Idempotently requests deterministic shutdown of all runtime components.
         *
         * @details Inbound work is stopped before dependent outbound/background resources are
         * destroyed. Safe to call before destruction and more than once.
         */
        void Stop() noexcept;

    private:
        /** Private implementation containing concrete core and adapter instances. */
        struct Impl;
        /** Sole owner of the process dependency graph. */
        std::unique_ptr<Impl> m_impl;
    };

}  // namespace edge::api_gateway::runtime
