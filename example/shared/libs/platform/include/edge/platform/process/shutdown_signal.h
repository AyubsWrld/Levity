/**
 * @file shutdown_signal.h
 * @brief Synchronous SIGINT/SIGTERM handling for service process main threads.
 */
#pragma once

#include <pthread.h>
#include <signal.h>

#include <cstring>
#include <stdexcept>
#include <string>

namespace edge::platform::process {

    /**
     * @brief Blocks termination signals and lets the main thread wait synchronously with sigwait().
     *
     * @details Construct this object before starting service worker threads. POSIX threads inherit
     * the creator's signal mask, so SIGINT/SIGTERM remain blocked in workers and can be consumed in a
     * deterministic, ordinary control-flow path by Wait(). This avoids async-signal-handler globals
     * and polling loops. Destruction restores the constructing thread's previous signal mask.
     *
     * @warning The intended lifecycle is one instance on the process/main thread. Do not destroy the
     * object while worker-thread assumptions about the inherited mask are still needed.
     */
    class Shutdown_Signal final {
    public:
        /**
         * @brief Blocks SIGINT and SIGTERM on the constructing thread and saves the previous mask.
         * @throws std::runtime_error if pthread_sigmask() fails.
         */
        Shutdown_Signal() {
            ::sigemptyset( &m_signal_set );
            ::sigaddset( &m_signal_set, SIGINT );
            ::sigaddset( &m_signal_set, SIGTERM );

            const int result = ::pthread_sigmask( SIG_BLOCK, &m_signal_set, &m_previous_mask );
            if( result != 0 ) {
                throw std::runtime_error( "Shutdown_Signal: pthread_sigmask() failed: " +
                                          std::string( std::strerror( result ) ) );
            }
            m_mask_installed = true;
        }

        /** @brief Restores the previous signal mask on the constructing thread. */
        ~Shutdown_Signal() {
            if( m_mask_installed ) {
                ::pthread_sigmask( SIG_SETMASK, &m_previous_mask, nullptr );
            }
        }

        Shutdown_Signal( const Shutdown_Signal& ) = delete;
        auto operator=( const Shutdown_Signal& ) -> Shutdown_Signal& = delete;
        Shutdown_Signal( Shutdown_Signal&& ) = delete;
        auto operator=( Shutdown_Signal&& ) -> Shutdown_Signal& = delete;

        /**
         * @brief Blocks until SIGINT or SIGTERM is delivered to the blocked signal set.
         * @return The received POSIX signal number.
         * @throws std::runtime_error if sigwait() fails.
         */
        [[nodiscard]] auto Wait() const -> int {
            int signal_number = 0;
            const int result = ::sigwait( &m_signal_set, &signal_number );
            if( result != 0 ) {
                throw std::runtime_error( "Shutdown_Signal: sigwait() failed: " +
                                          std::string( std::strerror( result ) ) );
            }
            return signal_number;
        }

    private:
        /** Signal set consumed synchronously by Wait(). */
        sigset_t m_signal_set {};
        /** Original thread signal mask restored on destruction. */
        sigset_t m_previous_mask {};
        /** Guards restoration if construction failed before installing the mask. */
        bool m_mask_installed = false;
    };

}  // namespace edge::platform::process
