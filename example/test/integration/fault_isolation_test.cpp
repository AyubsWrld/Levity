/**
 * @file fault_isolation_test.cpp
 * @brief Test implementation for fault isolation test behavior and regression coverage.
 *
 * @details Source path: `test/integration/fault_isolation_test.cpp`.
 */

#include "poll_until.h"
#include "test_harness.h"

#include <edge/ipc/context.h>
#include <edge/ipc/publisher.h>
#include <gtest/gtest.h>
#include <telemetry.pb.h>

#include <array>
#include <chrono>
#include <string>
#include <string_view>

// Mandatory fault-isolation acceptance case. Recovery is proven through public behavior, not
// internal counters: after the monolith crash a temporary test publisher changes the gateway cache
// to a sentinel value, then the restarted monolith must replace that sentinel with its normal
// deterministic telemetry again.
namespace edge::integration_test {
    namespace {

        constexpr auto READY_TIMEOUT = std::chrono::seconds( 10 );
        constexpr auto TELEMETRY_TIMEOUT = std::chrono::seconds( 10 );
        constexpr auto RESTART_RECOVERY_TIMEOUT = std::chrono::seconds( 10 );
        /** Sentinel coordinates published after killing the monolith to prove cache replacement. */
        constexpr double SENTINEL_LATITUDE_DEG = 1.0;
        constexpr double SENTINEL_LONGITUDE_DEG = 2.0;
        constexpr double SENTINEL_ALTITUDE_M = 3.0;

        /** @brief Fixture for the architectural requirement that control-plane services survive monolith failure. */
        class Fault_Isolation_Test : public ::testing::Test {
        protected:
            // GTest fixtures conventionally expose state via `protected` so that TEST_F-generated
            // subclasses can access it directly; there is no external caller to encapsulate against.
            // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
            support::Test_Harness harness;

            /** @brief Starts all services and proves normal readiness/telemetry before injecting a failure. */
            void Reach_Normal_Three_Process_Operation() {
                harness.Start_Gimbal();
                ASSERT_TRUE( harness.Wait_For_Gimbal_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();
                harness.Start_Gateway();
                ASSERT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();
                harness.Start_Monolith();
                ASSERT_TRUE( harness.Wait_For_Deterministic_Telemetry( TELEMETRY_TIMEOUT ) ) << harness.Diagnostics();
            }
        };

        /** @test Verifies that killing monolith leaves control plane alive and telemetry recovers. */
        TEST_F( Fault_Isolation_Test, Killing_Monolith_Leaves_Control_Plane_Alive_And_Telemetry_Recovers ) {
            Reach_Normal_Three_Process_Operation();

            harness.Kill_Monolith();
            ASSERT_FALSE( harness.Monolith_Alive() );

            ASSERT_TRUE( harness.Gateway_Alive() )
                << "api_gateway must survive an unrelated data-plane crash;" << harness.Diagnostics();
            ASSERT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) )
                << "api_gateway must remain HTTP-responsive after the monolith is killed;" << harness.Diagnostics();
            ASSERT_TRUE( harness.Gimbal_Alive() )
                << "gimbal_controller must be unaffected by the monolith crash;" << harness.Diagnostics();

            // Replace the stale deterministic cache with a sentinel cue using the same public
            // telemetry IPC surface. This gives the test an observable state change without adding
            // production metrics solely for integration-test bookkeeping.
            harness.Unlink_Telemetry_Socket_File();
            {
                edge::ipc::Context context;
                auto publisher_result = edge::ipc::Publisher::Bind( context, harness.Telemetry_Endpoint() );
                ASSERT_TRUE( publisher_result.has_value() ) << publisher_result.error().detail;
                auto& publisher = *publisher_result;

                edge::telemetry::v1::CuePriorityList sentinel_list;
                auto* sentinel = sentinel_list.add_targets();
                sentinel->set_uid( "fault_isolation_sentinel" );
                sentinel->set_latitude_deg( SENTINEL_LATITUDE_DEG );
                sentinel->set_longitude_deg( SENTINEL_LONGITUDE_DEG );
                sentinel->set_altitude_m( SENTINEL_ALTITUDE_M );

                std::string bytes;
                ASSERT_TRUE( sentinel_list.SerializeToString( &bytes ) );
                constexpr std::array<std::string_view, 1> SENTINEL_UIDS { "fault_isolation_sentinel" };

                const auto deadline = std::chrono::steady_clock::now() + RESTART_RECOVERY_TIMEOUT;
                ASSERT_TRUE( support::Poll_Until( deadline,
                                                  std::chrono::milliseconds( 20 ),
                                                  [&] {
                                                      const auto publish_result = publisher.Publish( "cues", bytes );
                                                      return publish_result.has_value() &&
                                                             harness.Cues_Match( SENTINEL_UIDS );
                                                  } ) )
                    << "gateway never ingested the sentinel telemetry after the monolith crash;"
                    << harness.Diagnostics();
            }

            // The test owns the temporary IPC directory and may remove a stale bind path before
            // launching the real publisher again. The service/library itself never unlinks an
            // existing endpoint it did not create.
            harness.Unlink_Telemetry_Socket_File();
            harness.Start_Monolith();

            ASSERT_TRUE( harness.Wait_For_Deterministic_Telemetry( RESTART_RECOVERY_TIMEOUT ) )
                << "restarted dummy_monolith never replaced sentinel telemetry with its normal cues;"
                << harness.Diagnostics();
        }

        /** @test Verifies that slew for cached uid during monolith outage does not crash gateway. */
        TEST_F( Fault_Isolation_Test, Slew_For_Cached_Uid_During_Monolith_Outage_Does_Not_Crash_Gateway ) {
            Reach_Normal_Three_Process_Operation();

            harness.Kill_Monolith();
            ASSERT_FALSE( harness.Monolith_Alive() );
            ASSERT_TRUE( harness.Gateway_Alive() ) << harness.Diagnostics();

            const auto response = harness.Post( "/api/v1/slew", R"({"target_uid":"drone_01"})" );
            ASSERT_TRUE( response ) << "gateway must still answer HTTP requests during a monolith outage;"
                                    << harness.Diagnostics();
            EXPECT_GE( response->status, 200 );
            EXPECT_LT( response->status, 600 );

            EXPECT_TRUE( harness.Gateway_Alive() )
                << "gateway must not crash from a slew during a monolith outage;" << harness.Diagnostics();
            EXPECT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) )
                << "gateway must remain HTTP-responsive after that slew;" << harness.Diagnostics();
        }

    }  // namespace
}  // namespace edge::integration_test
