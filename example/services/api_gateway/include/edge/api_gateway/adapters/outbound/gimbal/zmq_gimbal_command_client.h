/**
 * @file zmq_gimbal_command_client.h
 * @brief ZeroMQ REQ implementation of the API Gateway outbound gimbal-command port.
 */
#pragma once

#include <edge/api_gateway/core/ports/outbound/gimbal_command_port.h>
#include <edge/result.h>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <future>
#include <mutex>
#include <string>
#include <thread>

namespace edge::ipc {
    class Context;
}  // namespace edge::ipc

namespace edge::api_gateway::adapters::outbound::gimbal {

    /** Maximum number of caller requests buffered for the single ZeroMQ socket-owning worker. */
    inline constexpr std::size_t COMMAND_QUEUE_CAPACITY = 8;
    /** Request/reply deadline applied by the ZeroMQ requester for one command. */
    inline constexpr std::chrono::milliseconds GIMBAL_REQUEST_TIMEOUT { 1000 };
    /** Maximum time a caller waits for its queued work item to complete before returning failure. */
    inline constexpr std::chrono::milliseconds CALLER_WAIT_TIMEOUT { 1500 };

    /**
     * @brief Thread-safe queued adapter implementing Gimbal_Command_Port over ZeroMQ REQ/REP.
     *
     * @details Callers may originate from multiple REST/gRPC worker threads, but a ZeroMQ socket may
     * only be used by its owning thread. Send_Slew() therefore enqueues a Work_Item and waits on its
     * future, while one std::jthread owns the Requester and performs all ZeroMQ operations serially.
     * Queue capacity and waits are bounded to avoid unbounded memory growth or shutdown deadlock.
     *
     * A request timeout is surfaced as an ambiguous outcome and is never automatically retried. A
     * work item that expires while still queued is discarded without being sent, preventing stale
     * physical commands from executing after the caller has already received a timeout. The shared
     * Context must outlive the adapter and its worker.
     */
    class Zmq_Gimbal_Command_Client final : public core::ports::outbound::Gimbal_Command_Port {
    public:
        /**
         * @brief Starts the socket-owning command worker and connects it to @p endpoint.
         * @param context Shared ZeroMQ context that outlives this adapter.
         * @param endpoint Gimbal-command REQ endpoint.
         * @throws std::runtime_error when transport initialization is attempted but cannot create/connect the
         * requester. Standard exceptions from constructing the local thread/promise machinery may also propagate.
         * Exceptions raised while creating the native IPC wrapper on the worker are converted into
         * the synchronous startup failure rather than escaping the worker thread.
         */
        Zmq_Gimbal_Command_Client( edge::ipc::Context& context, std::string endpoint );

        /** @brief Stops the command worker and fails/drains outstanding queued work. */
        ~Zmq_Gimbal_Command_Client() override;

        Zmq_Gimbal_Command_Client( const Zmq_Gimbal_Command_Client& ) = delete;
        auto operator=( const Zmq_Gimbal_Command_Client& ) -> Zmq_Gimbal_Command_Client& = delete;
        Zmq_Gimbal_Command_Client( Zmq_Gimbal_Command_Client&& ) = delete;
        auto operator=( Zmq_Gimbal_Command_Client&& ) -> Zmq_Gimbal_Command_Client& = delete;

        /** @copydoc core::ports::outbound::Gimbal_Command_Port::Send_Slew() */
        [[nodiscard]] auto Send_Slew( const edge::commands::v1::SlewCommand& command )
            -> edge::Result<edge::commands::v1::SlewCommandReply, core::ports::outbound::Gimbal_Command_Error> override;

        /** @brief Idempotently requests stop, wakes the queue, and joins the worker. */
        void Stop() noexcept;

    private:
        /** Internal result carried through each work item's promise/future. */
        using Reply_Result =
            edge::Result<edge::commands::v1::SlewCommandReply, core::ports::outbound::Gimbal_Command_Error>;
        /** Startup handshake result communicated from the worker to the constructor. */
        using Startup_Result = edge::Result<void, std::string>;

        /** @brief One command, its absolute caller deadline, and the completion channel. */
        struct Work_Item {
            /** Domain command copied into the bounded queue. */
            edge::commands::v1::SlewCommand command;
            /** Absolute deadline after which an unsent queued command must be discarded. */
            std::chrono::steady_clock::time_point deadline;
            /** One-shot completion promise fulfilled by the socket-owning worker. */
            std::promise<Reply_Result> promise;
        };

        /** @brief Worker loop that exclusively owns and uses the ZeroMQ Requester. */
        void Run( std::stop_token stop_token,
                  edge::ipc::Context& context,
                  const std::string& endpoint,
                  std::promise<Startup_Result>& ready_promise );

        /** Protects queue contents and shutdown state. */
        std::mutex m_mutex;
        /** Wakes the worker when work arrives or shutdown begins. */
        std::condition_variable m_queue_not_empty;
        /** Bounded FIFO of caller requests awaiting the ZeroMQ worker. */
        std::deque<Work_Item> m_queue;
        /** Prevents new work from being admitted once shutdown starts. */
        bool m_shutting_down = false;
        /** Owns the single ZeroMQ socket thread. */
        std::jthread m_worker;
    };

}  // namespace edge::api_gateway::adapters::outbound::gimbal
