/**
 * @file zmq_command_server.h
 * @brief ZeroMQ REP inbound adapter for Gimbal Controller domain commands.
 */
#pragma once

#include <edge/gimbal_controller/core/ports/inbound/command_port.h>
#include <edge/platform/logging/logger.h>
#include <edge/result.h>

#include <future>
#include <string>
#include <thread>

namespace edge::ipc {
    class Context;
}  // namespace edge::ipc

namespace edge::gimbal_controller::adapters::inbound::commands {

    /**
     * @brief Receives serialized SlewCommand requests and invokes Command_Port.
     *
     * @details The adapter owns one std::jthread that exclusively creates/uses/destroys the ZeroMQ
     * REP socket. Parsed protobuf commands are passed to the transport-agnostic core inbound port,
     * and the returned domain reply is serialized back to the requester. Bounded receive waits allow
     * deterministic shutdown without detached threads or indefinitely blocked receives.
     *
     * Context, Command_Port, and Logger are non-owning dependencies that must outlive the adapter.
     */
    class Zmq_Command_Server final {
    public:
        /**
         * @brief Starts the command REP worker and synchronously verifies socket initialization.
         * @param context Shared ZeroMQ context that outlives this adapter.
         * @param endpoint Command endpoint to bind.
         * @param commands Core inbound command port.
         * @param logger Process logger for transport/boundary diagnostics.
         * @throws std::runtime_error when transport initialization is attempted but cannot bind its replier.
         * Standard exceptions from constructing the local thread/promise machinery may also propagate.
         * Exceptions raised while creating the native IPC wrapper on the worker are converted into
         * the synchronous startup failure rather than escaping the worker thread.
         */
        Zmq_Command_Server( edge::ipc::Context& context,
                            std::string endpoint,
                            core::ports::inbound::Command_Port& commands,
                            edge::platform::logging::Logger& logger );

        /** @brief Stops and joins the command worker. */
        ~Zmq_Command_Server();

        Zmq_Command_Server( const Zmq_Command_Server& ) = delete;
        auto operator=( const Zmq_Command_Server& ) -> Zmq_Command_Server& = delete;
        Zmq_Command_Server( Zmq_Command_Server&& ) = delete;
        auto operator=( Zmq_Command_Server&& ) -> Zmq_Command_Server& = delete;

        /** @brief Idempotently requests stop and joins the socket-owning worker. */
        void Stop() noexcept;

    private:
        /** One-shot startup handshake result. */
        using Startup_Result = edge::Result<void, std::string>;

        /** @brief REP socket worker that processes request/reply cycles serially. */
        static void Run( edge::ipc::Context& context,
                         const std::string& endpoint,
                         core::ports::inbound::Command_Port& commands,
                         edge::platform::logging::Logger& logger,
                         const std::stop_token& stop_token,
                         std::promise<Startup_Result>& ready_promise );

        /** Owns the thread to which the ZeroMQ REP socket is affined. */
        std::jthread m_worker;
    };

}  // namespace edge::gimbal_controller::adapters::inbound::commands
