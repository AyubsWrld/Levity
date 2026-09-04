/**
 * @file rest_server.h
 * @brief REST inbound adapter exposing the API Gateway application port over HTTP/JSON.
 */
#pragma once

#include <edge/api_gateway/core/ports/inbound/api_port.h>
#include <edge/platform/logging/logger.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace edge::api_gateway::adapters::inbound::rest {

    /** Maximum accepted REST request-body size, limiting attacker-controlled allocation/work. */
    inline constexpr std::size_t MAX_REST_BODY_BYTES = 4096;
    /** Fixed HTTP worker count used to bound service concurrency and memory on edge hardware. */
    inline constexpr std::size_t REST_WORKER_THREADS = 4;
    /** Maximum queued HTTP requests awaiting a worker. */
    inline constexpr std::size_t MAX_QUEUED_REQUESTS = 64;

    /**
     * @brief HTTP/JSON adapter for operator-facing API Gateway use cases.
     *
     * @details The adapter performs HTTP parsing, request-size enforcement, JSON translation, and
     * mapping of core outcomes to HTTP responses. It depends only on Api_Port and therefore does not
     * know about Cue_Cache, concrete core services, or the ZeroMQ gimbal adapter.
     *
     * The adapter owns its HTTP server implementation and worker lifecycle. The referenced Api_Port
     * and Logger are non-owning and must outlive this object; the runtime composition root enforces
     * that ordering.
     */
    class Rest_Server final {
    public:
        /**
         * @brief Constructs and starts the REST listener.
         * @param host Interface/address to bind.
         * @param port TCP port to bind.
         * @param api Application-facing inbound port invoked by request handlers.
         * @param logger Non-owning process logger.
         * @throws std::runtime_error if the HTTP endpoint cannot be bound. Standard allocation/thread-start
         * exceptions may also propagate during construction.
         */
        Rest_Server( std::string host,
                     std::uint16_t port,
                     core::ports::inbound::Api_Port& api,
                     edge::platform::logging::Logger& logger );

        /** @brief Stops the listener/workers and releases HTTP resources. */
        ~Rest_Server();

        Rest_Server( const Rest_Server& ) = delete;
        auto operator=( const Rest_Server& ) -> Rest_Server& = delete;
        Rest_Server( Rest_Server&& ) = delete;
        auto operator=( Rest_Server&& ) -> Rest_Server& = delete;

        /** @brief Returns the TCP port actually bound by the HTTP server. */
        [[nodiscard]] auto Bound_Port() const noexcept -> std::uint16_t;

        /** @brief Idempotently stops accepting/processing HTTP requests. */
        void Stop() noexcept;

    private:
        /** Transport-specific implementation hidden from public headers. */
        struct Impl;
        /** Owns listener state and HTTP worker resources. */
        std::unique_ptr<Impl> m_impl;
    };

}  // namespace edge::api_gateway::adapters::inbound::rest
