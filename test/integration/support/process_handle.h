/**
 * @file process_handle.h
 * @brief RAII child-process control and captured-output support for black-box integration tests.
 */
#pragma once

#include <edge/platform/posix/unique_fd.h>
#include <sys/types.h>

#include <chrono>
#include <string>
#include <vector>

namespace edge::integration_test::support {

    /**
     * @brief Owns one launched service process and performs deterministic termination/reaping at scope exit.
     *
     * @details Construction forks and `execve()`s a built executable. The child inherits the parent
     * environment and then applies the supplied name/value overrides, preserving runtime loader,
     * sanitizer, and toolchain variables that the built binary may require. Combined stdout/stderr is
     * captured through a non-blocking pipe and stored for assertion
     * diagnostics. Under normal POSIX process-control behavior, the destructor prevents
     * zombies/orphans by sending SIGTERM, escalating to SIGKILL after a bounded grace period when
     * needed, and reaping the child. Unexpected kernel-level waitpid() failures remain best-effort
     * because a destructor cannot safely report them to the test body.
     */
    class Process_Handle final {
    public:
        /** @brief One environment name/value pair supplied to the child process. */
        struct Environment_Variable {
            /** Environment variable name. */
            std::string name;
            /** Environment variable value. */
            std::string value;
        };

        /**
         * @brief Launches one child executable with inherited environment plus explicit overrides.
         * @param binary_path Path to the built executable passed to execve().
         * @param environment Child-specific variables that replace/add entries in the inherited environment.
         * @param label Human-readable role used only in diagnostics.
         * @throws std::runtime_error for invalid override names or if parent-side pipe/fork/non-blocking-pipe setup
         * fails. Child-side stdio setup failure is observable as exit status 126 and `execve()` failure as exit status
         * 127, rather than as constructor exceptions.
         */
        Process_Handle( const std::string& binary_path,
                        const std::vector<Environment_Variable>& environment,
                        std::string label );

        /** @brief Terminates and reaps the child if still owned/alive. */
        ~Process_Handle();

        Process_Handle( const Process_Handle& ) = delete;
        auto operator=( const Process_Handle& ) -> Process_Handle& = delete;
        /** @brief Transfers ownership of the child process and capture pipe. */
        Process_Handle( Process_Handle&& other ) noexcept;
        /** @brief Terminates current ownership, then transfers the child from @p other. */
        auto operator=( Process_Handle&& other ) noexcept -> Process_Handle&;

        /**
         * @brief Performs a non-blocking liveness check and reaps an exited child when observed.
         * @return true while the child is running, false once it has exited or ownership is empty.
         */
        [[nodiscard]] auto Is_Alive() -> bool;

        /**
         * @brief Requests graceful termination and escalates to SIGKILL after a bounded deadline.
         * @note Idempotent and safe after natural child exit.
         */
        void Terminate();

        /**
         * @brief Immediately sends SIGKILL and reaps the child.
         * @details Reaping is polled with a bounded deadline first, followed by a last-resort blocking
         * wait to avoid leaving a zombie if scheduling delays exceed that deadline.
         */
        void Kill();

        /**
         * @brief Polls child liveness until exit or @p deadline.
         * @return true when the child exited/reaped before the deadline, otherwise false.
         */
        [[nodiscard]] auto Wait_For_Exit( std::chrono::steady_clock::time_point deadline ) -> bool;

        /**
         * @brief Drains currently available stdout/stderr bytes and returns the accumulated buffer.
         * @return Reference valid for this handle's lifetime and until further draining mutates the string.
         */
        [[nodiscard]] auto Drain_Output() -> const std::string&;

        /** @brief Returns the owned child PID, or -1 when no child is owned. */
        [[nodiscard]] auto Pid() const noexcept -> pid_t;
        /** @brief Returns the human-readable role label. */
        [[nodiscard]] auto Label() const noexcept -> const std::string&;

    private:
        /** @brief Reads all currently available bytes from the non-blocking capture pipe. */
        void Pump_Pipe();
        /** @brief Performs the final bounded/blocking reap path once termination has been requested. */
        void Reap_Blocking();

        /** Owned child PID or -1 for an empty/moved-from handle. */
        pid_t m_pid = -1;
        /** Read end of the combined stdout/stderr capture pipe. */
        edge::platform::posix::Unique_Fd m_stdout_read_fd;
        /** Role label included in failure diagnostics. */
        std::string m_label;
        /** Complete captured stdout/stderr collected so far. */
        std::string m_captured_output;
        /** Guards against double waitpid() after the child has already been reaped. */
        bool m_reaped = true;
    };

}  // namespace edge::integration_test::support
