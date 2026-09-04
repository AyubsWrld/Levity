/**
 * @file restart_recovery_test.cpp
 * @brief Test implementation for restart recovery test behavior and regression coverage.
 *
 * @details Source path: `test/integration/restart_recovery_test.cpp`.
 */

#include "test_harness.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <string>

// Restart/recovery coverage (.github/bootstrap-mvp.md Phase 6 negative-path matrix: "restart /
// recovery"). Each scenario restarts exactly one process on the same endpoint/configuration and
// proves the system recovers, without asserting anything about the deliberately-unspecified
// stale-cache semantics (docs/architecture/tier1-contracts.md §6).
namespace edge::integration_test {
    namespace {

        constexpr auto READY_TIMEOUT = std::chrono::seconds( 10 );
        constexpr auto TELEMETRY_TIMEOUT = std::chrono::seconds( 10 );
        constexpr auto UDP_TIMEOUT = std::chrono::seconds( 3 );
        constexpr auto RESTART_GRACE_TIMEOUT = std::chrono::seconds( 10 );

        /** @brief Fixture for service restart/reconnect behavior using the real three-process topology. */
        class Restart_Recovery_Test : public ::testing::Test {
        protected:
            support::Test_Harness harness;
        };

        /** @test Verifies that gimbal restart on same endpoint allows slew to succeed again. */
        TEST_F( Restart_Recovery_Test, Gimbal_Restart_On_Same_Endpoint_Allows_Slew_To_Succeed_Again ) {
            harness.Start_Gimbal();
            ASSERT_TRUE( harness.Wait_For_Gimbal_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();
            harness.Start_Gateway();
            ASSERT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();
            harness.Start_Monolith();
            ASSERT_TRUE( harness.Wait_For_Deterministic_Telemetry( TELEMETRY_TIMEOUT ) ) << harness.Diagnostics();

            const auto first_slew = harness.Post( "/api/v1/slew", R"({"target_uid":"drone_01"})" );
            ASSERT_TRUE( first_slew ) << harness.Diagnostics();
            ASSERT_EQ( first_slew->status, 200 ) << harness.Diagnostics();
            ASSERT_TRUE( harness.Udp().Receive_Datagram( UDP_TIMEOUT ).has_value() ) << harness.Diagnostics();
            ASSERT_TRUE( harness.Udp().Receive_Datagram( UDP_TIMEOUT ).has_value() ) << harness.Diagnostics();

            // A graceful SIGTERM shutdown is expected to release the ipc:// bind path itself; the
            // harness-owned unlink is still performed defensively (it owns the temp directory, and a
            // stale socket file must never be left for the service to work around).
            harness.Stop_Gimbal();
            harness.Unlink_Gimbal_Socket_File();

            harness.Start_Gimbal();
            ASSERT_TRUE( harness.Wait_For_Gimbal_Ready( RESTART_GRACE_TIMEOUT ) )
                << "gimbal_controller failed to rebind on the same endpoint after restart;" << harness.Diagnostics();

            const auto second_slew = harness.Post( "/api/v1/slew", R"({"target_uid":"drone_02"})" );
            ASSERT_TRUE( second_slew ) << harness.Diagnostics();
            ASSERT_EQ( second_slew->status, 200 ) << harness.Diagnostics();

            const auto datagram = harness.Udp().Receive_Datagram( UDP_TIMEOUT );
            ASSERT_TRUE( datagram.has_value() ) << "no UDP output after gimbal restart;" << harness.Diagnostics();
            ASSERT_TRUE( harness.Udp().Receive_Datagram( UDP_TIMEOUT ).has_value() ) << harness.Diagnostics();
        }

        /** @test Verifies that gateway restart on same configuration resumes telemetry ingestion. */
        TEST_F( Restart_Recovery_Test, Gateway_Restart_On_Same_Configuration_Resumes_Telemetry_Ingestion ) {
            harness.Start_Gateway();
            ASSERT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();
            harness.Start_Monolith();
            ASSERT_TRUE( harness.Wait_For_Deterministic_Telemetry( TELEMETRY_TIMEOUT ) ) << harness.Diagnostics();

            harness.Stop_Gateway();
            EXPECT_FALSE( harness.Gateway_Alive() );

            harness.Start_Gateway();
            ASSERT_TRUE( harness.Wait_For_Gateway_Ready( RESTART_GRACE_TIMEOUT ) )
                << "api_gateway failed to become ready again after restart;" << harness.Diagnostics();

            ASSERT_TRUE( harness.Wait_For_Deterministic_Telemetry( TELEMETRY_TIMEOUT ) )
                << "a freshly restarted gateway never re-subscribed and repopulated /api/v1/cues;"
                << harness.Diagnostics();
        }

    }  // namespace
}  // namespace edge::integration_test
