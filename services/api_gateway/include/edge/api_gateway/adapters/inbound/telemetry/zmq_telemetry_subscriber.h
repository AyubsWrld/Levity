/**
 * @file zmq_telemetry_subscriber.h
 * @brief ZeroMQ SUB inbound adapter that feeds telemetry domain messages into Telemetry_Port.
 */
#pragma once

#include <edge/api_gateway/core/ports/inbound/telemetry_port.h>
#include <edge/platform/logging/logger.h>
#include <edge/result.h>

#include <future>
#include <string>
#include <thread>

namespace edge::ipc {
    class Context;
}  // namespace edge::ipc

namespace edge::api_gateway::adapters::inbound::telemetry {

    /**
     * @brief Owns the telemetry SUB worker and translates ZeroMQ payloads into core telemetry updates.
     *
     * @details The ZeroMQ socket is created, used, and destroyed exclusively on the worker thread to
     * obey ZeroMQ thread-affinity requirements. Construction waits for a startup promise so failure
     * to connect/create the essential subscriber is reported synchronously rather than leaving the
     * process apparently healthy with a dead telemetry thread.
     *
     * The Context, Telemetry_Port, and Logger references are non-owning and must outlive this adapter.
     */
    class Zmq_Telemetry_Subscriber final {
    public:
        /**
         * @brief Starts the telemetry subscription worker.
         * @param context Shared ZeroMQ context whose lifetime exceeds the worker/socket lifetime.
         * @param endpoint Configured telemetry endpoint to connect to.
         * @param telemetry Core inbound port receiving successfully parsed CuePriorityList snapshots.
         * @param logger Process logger for boundary/transport diagnostics.
         * @throws std::runtime_error when transport initialization is attempted but cannot create/connect the
         * subscriber. Standard exceptions from constructing the local thread/promise machinery may also propagate.
         * Exceptions raised while creating the native IPC wrapper on the worker are converted into
         * the synchronous startup failure rather than escaping the worker thread.
         */
        Zmq_Telemetry_Subscriber( edge::ipc::Context& context,
                                  std::string endpoint,
                                  core::ports::inbound::Telemetry_Port& telemetry,
                                  edge::platform::logging::Logger& logger );

        /** @brief Requests worker shutdown and joins the worker thread. */
        ~Zmq_Telemetry_Subscriber();

        Zmq_Telemetry_Subscriber( const Zmq_Telemetry_Subscriber& ) = delete;
        auto operator=( const Zmq_Telemetry_Subscriber& ) -> Zmq_Telemetry_Subscriber& = delete;
        Zmq_Telemetry_Subscriber( Zmq_Telemetry_Subscriber&& ) = delete;
        auto operator=( Zmq_Telemetry_Subscriber&& ) -> Zmq_Telemetry_Subscriber& = delete;

        /** @brief Idempotently requests stop and joins the telemetry worker. */
        void Stop() noexcept;

    private:
        /** Startup handshake result communicated from the socket-owning worker to the constructor. */
        using Startup_Result = edge::Result<void, std::string>;

        /**
         * @brief Socket-owning worker loop.
         * @param stop_token Cooperative shutdown token supplied by std::jthread.
         * @param context Shared ZeroMQ context.
         * @param endpoint Endpoint copied into the adapter and stable for the worker lifetime.
         * @param telemetry Core inbound port receiving successfully parsed snapshots.
         * @param logger Logger used for parse/transport/recovery diagnostics.
         * @param ready_promise One-shot startup handshake fulfilled after socket initialization.
         */
        static void Run( const std::stop_token& stop_token,
                         edge::ipc::Context& context,
                         const std::string& endpoint,
                         core::ports::inbound::Telemetry_Port& telemetry,
                         edge::platform::logging::Logger& logger,
                         std::promise<Startup_Result>& ready_promise );

        /** Sole owner of the ZeroMQ subscriber worker lifecycle. */
        std::jthread m_worker;
    };

}  // namespace edge::api_gateway::adapters::inbound::telemetry
