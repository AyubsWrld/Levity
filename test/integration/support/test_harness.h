/**
 * @file test_harness.h
 * @brief Black-box multi-process integration-test harness for the Tier-1 service topology.
 */
#pragma once

#include "process_handle.h"
#include "udp_listener.h"

#include <edge/test/temp_dir.h>
#include <httplib/httplib.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace edge::integration_test::support {

    /**
     * @brief Finds a currently free loopback TCP port for a subsequently launched HTTP listener.
     * @return Port selected by binding `127.0.0.1:0` and querying getsockname().
     *
     * @warning The probe socket is closed before the real API Gateway binds, so this is inherently a
     * best-effort TCP-port allocation. UDP avoids this race by keeping Udp_Listener bound throughout.
     */
    [[nodiscard]] auto Allocate_Free_Tcp_Port() -> std::uint16_t;

    /**
     * @brief Owns all isolated resources and child processes for one black-box integration test.
     *
     * @details Each harness instance creates a short unique IPC directory/endpoints, selects an HTTP
     * port, binds a UDP observation socket, and optionally launches gimbal, gateway, and dummy-monolith
     * processes with explicit environment variables. Readiness helpers use bounded observable polling
     * and verify process liveness so a crash is diagnosed directly rather than surfacing only as a
     * later transport timeout.
     *
     * Destruction requests termination and reaps every owned child before removing the temporary
     * directory. The graceful termination phase is bounded and escalates to SIGKILL; the final
     * blocking reap can still wait under pathological uninterruptible-kernel conditions.
     * Stale IPC socket unlinking is deliberately owned by the test harness after intentionally killing
     * a binder; production services never blindly unlink configured endpoint files.
     */
    class Test_Harness final {
    public:
        /** @brief Allocates isolated endpoints, HTTP port, UDP listener, and HTTP client state. */
        Test_Harness();
        /** @brief Stops all launched processes and releases test resources. */
        ~Test_Harness();

        Test_Harness( const Test_Harness& ) = delete;
        auto operator=( const Test_Harness& ) -> Test_Harness& = delete;
        Test_Harness( Test_Harness&& ) = delete;
        auto operator=( Test_Harness&& ) -> Test_Harness& = delete;

        /** @brief Returns the unique telemetry PUB/SUB endpoint for this test. */
        [[nodiscard]] auto Telemetry_Endpoint() const -> const std::string&;
        /** @brief Returns the unique gimbal-command REQ/REP endpoint for this test. */
        [[nodiscard]] auto Gimbal_Endpoint() const -> const std::string&;
        /** @brief Returns the filesystem path component of Telemetry_Endpoint(). */
        [[nodiscard]] auto Telemetry_Socket_Path() const -> std::string;
        /** @brief Returns the filesystem path component of Gimbal_Endpoint(). */
        [[nodiscard]] auto Gimbal_Socket_Path() const -> std::string;
        /** @brief Returns the HTTP port configured for the gateway child. */
        [[nodiscard]] auto Http_Port() const noexcept -> std::uint16_t;
        /** @brief Returns the bound UDP capture listener used as the gimbal hardware destination. */
        [[nodiscard]] auto Udp() -> Udp_Listener&;

        /**
         * @brief Launches the Gimbal Controller with harness defaults plus optional overrides.
         * @param extra_environment Overrides/additions applied by variable name after harness defaults.
         */
        void Start_Gimbal( const std::vector<Process_Handle::Environment_Variable>& extra_environment = {} );
        /** @brief Launches the API Gateway with harness defaults plus optional overrides. */
        void Start_Gateway( const std::vector<Process_Handle::Environment_Variable>& extra_environment = {} );
        /** @brief Launches the deterministic dummy monolith with harness defaults plus optional overrides. */
        void Start_Monolith( const std::vector<Process_Handle::Environment_Variable>& extra_environment = {} );

        /** @brief Requests graceful gimbal termination, then escalates to SIGKILL after a bounded grace period. */
        void Stop_Gimbal();
        /** @brief Requests graceful gateway termination, then escalates to SIGKILL after a bounded grace period. */
        void Stop_Gateway();
        /** @brief Requests graceful monolith termination, then escalates to SIGKILL after a bounded grace period. */
        void Stop_Monolith();

        /** @brief Immediately SIGKILLs the monolith to simulate an unclean data-plane crash. */
        void Kill_Monolith();

        /** @brief Returns whether the currently owned gimbal child is alive. */
        [[nodiscard]] auto Gimbal_Alive() -> bool;
        /** @brief Returns whether the currently owned gateway child is alive. */
        [[nodiscard]] auto Gateway_Alive() -> bool;
        /** @brief Returns whether the currently owned monolith child is alive. */
        [[nodiscard]] auto Monolith_Alive() -> bool;

        /**
         * @brief Removes the telemetry UDS file after a test-owned unclean binder termination.
         * @note This is test-infrastructure ownership, not behavior expected from production services.
         */
        void Unlink_Telemetry_Socket_File() const;
        /** @brief Removes the gimbal-command UDS file after a test-owned unclean binder termination. */
        void Unlink_Gimbal_Socket_File() const;

        /**
         * @brief Waits until the gimbal command endpoint is observably ready or @p timeout elapses.
         * @return true only when ready while the child remains alive.
         */
        [[nodiscard]] auto Wait_For_Gimbal_Ready( std::chrono::milliseconds timeout ) -> bool;
        /** @brief Waits until the gateway HTTP readiness endpoint responds successfully. */
        [[nodiscard]] auto Wait_For_Gateway_Ready( std::chrono::milliseconds timeout ) -> bool;
        /** @brief Waits until `/api/v1/cues` exposes the deterministic dummy-monolith target set. */
        [[nodiscard]] auto Wait_For_Deterministic_Telemetry( std::chrono::milliseconds timeout ) -> bool;
        /**
         * @brief Checks whether the current public cue response contains exactly @p expected_uids in order.
         * @return false on transport/parse/status mismatch as well as UID mismatch.
         */
        [[nodiscard]] auto Cues_Match( std::span<const std::string_view> expected_uids ) -> bool;
        /** @brief Searches captured gateway stdout/stderr for @p text. */
        [[nodiscard]] auto Gateway_Output_Contains( std::string_view text ) -> bool;

        /**
         * @brief Formats captured output for all process roles for assertion diagnostics.
         * @return Diagnostic string containing live or most recently retained output for each role.
         */
        [[nodiscard]] auto Diagnostics() -> std::string;

        /** @brief Performs an HTTP GET against the configured gateway loopback listener. */
        [[nodiscard]] auto Get( const std::string& path ) -> httplib::Result;
        /**
         * @brief Performs an HTTP POST against the configured gateway listener.
         * @param path Request path.
         * @param body Raw request body.
         * @param content_type Content-Type header, defaulting to JSON.
         */
        [[nodiscard]] auto Post( const std::string& path,
                                 const std::string& body,
                                 const std::string& content_type = "application/json" ) -> httplib::Result;

    private:
        /** Unique short directory containing per-test IPC socket paths. */
        edge::test::Temp_Dir m_temp_dir;
        /** Unique telemetry endpoint derived from m_temp_dir. */
        std::string m_telemetry_endpoint;
        /** Unique gimbal-command endpoint derived from m_temp_dir. */
        std::string m_gimbal_endpoint;
        /** Loopback HTTP port assigned to the gateway child. */
        std::uint16_t m_http_port;
        /** Bound UDP observer and authoritative destination-port allocation. */
        Udp_Listener m_udp_listener;
        /** HTTP client targeting the gateway's configured loopback endpoint. */
        httplib::Client m_client;

        /** Currently owned gimbal child, when started. */
        std::optional<Process_Handle> m_gimbal;
        /** Currently owned gateway child, when started. */
        std::optional<Process_Handle> m_gateway;
        /** Currently owned monolith child, when started. */
        std::optional<Process_Handle> m_monolith;

        /** Output retained from the most recently stopped/replaced gimbal process. */
        std::string m_last_gimbal_output;
        /** Output retained from the most recently stopped/replaced gateway process. */
        std::string m_last_gateway_output;
        /** Output retained from the most recently stopped/replaced monolith process. */
        std::string m_last_monolith_output;
    };

}  // namespace edge::integration_test::support
