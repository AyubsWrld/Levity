/**
 * @file negative_paths_test.cpp
 * @brief Test implementation for negative paths test behavior and regression coverage.
 *
 * @details Source path: `test/integration/negative_paths_test.cpp`.
 */

#include "poll_until.h"
#include "test_harness.h"

#include <edge/ipc/context.h>
#include <edge/ipc/publisher.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <telemetry.pb.h>

#include <array>
#include <chrono>
#include <string>
#include <string_view>
#include <vector>

// Negative-path black-box coverage (docs/architecture/tier1-contracts.md §0/§9;
// .github/bootstrap-mvp.md Phase 6 "Initial negative-path integration coverage").
//
// tier1-contracts.md §0 explicitly defers semantic validation (coordinate ranges, UID length,
// duplicate-UID rejection): none of that is exercised here. tier1-contracts.md §6 leaves
// stale-cache semantics deliberately unspecified: none of that is asserted here either.
namespace edge::integration_test {
    namespace {

        constexpr auto READY_TIMEOUT = std::chrono::seconds( 10 );
        constexpr auto TELEMETRY_TIMEOUT = std::chrono::seconds( 10 );
        constexpr auto UDP_TIMEOUT = std::chrono::seconds( 3 );
        constexpr auto SHORT_UDP_SILENCE_WINDOW = std::chrono::milliseconds( 500 );
        constexpr auto TELEMETRY_OBSERVATION_TIMEOUT = std::chrono::seconds( 10 );
        /** HTTP status codes asserted on by the unavailable-gimbal-peer negative path below. */
        constexpr int HTTP_SERVICE_UNAVAILABLE = 503;
        constexpr int HTTP_GATEWAY_TIMEOUT = 504;
        /** Sample coordinates used by the recovery message published after malformed telemetry. */
        constexpr double SAMPLE_LATITUDE_DEG = 34.05;
        constexpr double SAMPLE_LONGITUDE_DEG = -118.25;
        constexpr double SAMPLE_ALTITUDE_M = 150.0;

        // tier1-contracts.md §7: MAX_REST_BODY_BYTES = 4096. Deliberately a local literal (not
        // included from the service's private adapter header - this is a black-box suite) with the
        // provenance recorded here instead of being an unexplained magic number.
        constexpr std::size_t MAX_REST_BODY_BYTES = 4096;

        /** @brief End-to-end fixture covering malformed input, unavailable peers, and recovery behavior. */
        class Negative_Paths_Test : public ::testing::Test {
        protected:
            support::Test_Harness harness;
        };

        /** @test Verifies that unknown target uid returns 404 and no udp datagram. */
        TEST_F( Negative_Paths_Test, Unknown_Target_Uid_Returns_404_And_No_Udp_Datagram ) {
            harness.Start_Gimbal();
            ASSERT_TRUE( harness.Wait_For_Gimbal_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();
            harness.Start_Gateway();
            ASSERT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();
            harness.Start_Monolith();
            ASSERT_TRUE( harness.Wait_For_Deterministic_Telemetry( TELEMETRY_TIMEOUT ) ) << harness.Diagnostics();

            const auto response = harness.Post( "/api/v1/slew", R"({"target_uid":"no_such_target"})" );
            ASSERT_TRUE( response ) << harness.Diagnostics();
            EXPECT_EQ( response->status, 404 );
            const auto body = nlohmann::json::parse( response->body );
            EXPECT_EQ( body.at( "error" ), "unknown_target" );
            EXPECT_EQ( body.at( "target_uid" ), "no_such_target" );

            const auto datagram = harness.Udp().Receive_Datagram( SHORT_UDP_SILENCE_WINDOW );
            EXPECT_FALSE( datagram.has_value() ) << "an unknown-target slew must never reach the gimbal/UDP";
        }

        /** @test Verifies that malformed json body returns 400 and service recovers for next slew. */
        TEST_F( Negative_Paths_Test, Malformed_Json_Body_Returns_400_And_Service_Recovers_For_Next_Slew ) {
            harness.Start_Gimbal();
            ASSERT_TRUE( harness.Wait_For_Gimbal_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();
            harness.Start_Gateway();
            ASSERT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();
            harness.Start_Monolith();
            ASSERT_TRUE( harness.Wait_For_Deterministic_Telemetry( TELEMETRY_TIMEOUT ) ) << harness.Diagnostics();

            const auto malformed_response = harness.Post( "/api/v1/slew", "not-json-at-all" );
            ASSERT_TRUE( malformed_response ) << harness.Diagnostics();
            EXPECT_EQ( malformed_response->status, 400 );
            const auto malformed_body = nlohmann::json::parse( malformed_response->body );
            EXPECT_EQ( malformed_body.at( "error" ), "invalid_request" );

            ASSERT_TRUE( harness.Gateway_Alive() ) << "gateway must survive a malformed request body";

            const auto valid_response = harness.Post( "/api/v1/slew", R"({"target_uid":"drone_01"})" );
            ASSERT_TRUE( valid_response ) << harness.Diagnostics();
            ASSERT_EQ( valid_response->status, 200 ) << harness.Diagnostics();

            EXPECT_TRUE( harness.Udp().Receive_Datagram( UDP_TIMEOUT ).has_value() )
                << "expected #200 after recovery;" << harness.Diagnostics();
            EXPECT_TRUE( harness.Udp().Receive_Datagram( UDP_TIMEOUT ).has_value() )
                << "expected #201 after recovery;" << harness.Diagnostics();
        }

        /** @test Verifies that missing empty and wrong type target uid return 400. */
        TEST_F( Negative_Paths_Test, Missing_Empty_And_Wrong_Type_Target_Uid_Return_400 ) {
            harness.Start_Gimbal();
            ASSERT_TRUE( harness.Wait_For_Gimbal_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();
            harness.Start_Gateway();
            ASSERT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();

            const std::vector<std::pair<std::string, std::string>> invalid_bodies {
                { "missing_target_uid", R"({})" },
                { "empty_target_uid", R"({"target_uid":""})" },
                { "non_string_target_uid", R"({"target_uid":123})" },
                { "non_object_json", R"([1,2,3])" },
            };

            for( const auto& [case_name, request_body] : invalid_bodies ) {
                const auto response = harness.Post( "/api/v1/slew", request_body );
                ASSERT_TRUE( response ) << case_name << ";" << harness.Diagnostics();
                EXPECT_EQ( response->status, 400 ) << "case=" << case_name;
                const auto body = nlohmann::json::parse( response->body );
                EXPECT_EQ( body.at( "error" ), "invalid_request" ) << "case=" << case_name;
            }
        }

        /** @test Verifies that oversized body returns 413. */
        TEST_F( Negative_Paths_Test, Oversized_Body_Returns_413 ) {
            harness.Start_Gimbal();
            ASSERT_TRUE( harness.Wait_For_Gimbal_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();
            harness.Start_Gateway();
            ASSERT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();

            const std::string oversized_padding( MAX_REST_BODY_BYTES + 1, 'a' );
            const auto oversized_body = nlohmann::json { { "target_uid", oversized_padding } }.dump();
            ASSERT_GT( oversized_body.size(), MAX_REST_BODY_BYTES );

            const auto response = harness.Post( "/api/v1/slew", oversized_body );
            ASSERT_TRUE( response ) << harness.Diagnostics();
            EXPECT_EQ( response->status, 413 );
            const auto body = nlohmann::json::parse( response->body );
            EXPECT_EQ( body.at( "error" ), "body_too_large" );

            ASSERT_TRUE( harness.Gateway_Alive() ) << "gateway must survive an oversized request body";
        }

        /** @test Verifies that unknown route returns 404. */
        TEST_F( Negative_Paths_Test, Unknown_Route_Returns_404 ) {
            harness.Start_Gateway();
            ASSERT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();

            const auto response = harness.Get( "/api/v1/does-not-exist" );
            ASSERT_TRUE( response ) << harness.Diagnostics();
            EXPECT_EQ( response->status, 404 );
        }

        /** @test Verifies that empty cache returns empty targets and 404 for any slew. */
        TEST_F( Negative_Paths_Test, Empty_Cache_Returns_Empty_Targets_And_404_For_Any_Slew ) {
            // Gateway + gimbal running, monolith never started: the cache must stay empty (never
            // 404 on GET /api/v1/cues) and any slew must fail with unknown_target.
            harness.Start_Gimbal();
            ASSERT_TRUE( harness.Wait_For_Gimbal_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();
            harness.Start_Gateway();
            ASSERT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();

            const auto cues_response = harness.Get( "/api/v1/cues" );
            ASSERT_TRUE( cues_response ) << harness.Diagnostics();
            EXPECT_EQ( cues_response->status, 200 );
            const auto cues_body = nlohmann::json::parse( cues_response->body );
            ASSERT_TRUE( cues_body.at( "targets" ).is_array() );
            EXPECT_TRUE( cues_body.at( "targets" ).empty() );

            const auto slew_response = harness.Post( "/api/v1/slew", R"({"target_uid":"drone_01"})" );
            ASSERT_TRUE( slew_response ) << harness.Diagnostics();
            EXPECT_EQ( slew_response->status, 404 );
            const auto slew_body = nlohmann::json::parse( slew_response->body );
            EXPECT_EQ( slew_body.at( "error" ), "unknown_target" );
        }

        /** @test Verifies that gimbal unavailable returns documented error and gateway stays responsive. */
        TEST_F( Negative_Paths_Test, Gimbal_Unavailable_Returns_Documented_Error_And_Gateway_Stays_Responsive ) {
            // Gateway + monolith running, gimbal_controller never started: the target is known (so
            // the failure is genuinely about the unreachable peer, not an unknown UID) but the
            // gimbal command channel has no peer bound at all.
            harness.Start_Gateway();
            ASSERT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();
            harness.Start_Monolith();
            ASSERT_TRUE( harness.Wait_For_Deterministic_Telemetry( TELEMETRY_TIMEOUT ) ) << harness.Diagnostics();

            const auto started_at = std::chrono::steady_clock::now();
            const auto response = harness.Post( "/api/v1/slew", R"({"target_uid":"drone_01"})" );
            const auto elapsed = std::chrono::steady_clock::now() - started_at;

            ASSERT_TRUE( response ) << harness.Diagnostics();
            EXPECT_TRUE( response->status == HTTP_SERVICE_UNAVAILABLE || response->status == HTTP_GATEWAY_TIMEOUT )
                << "unexpected status " << response->status << ";" << harness.Diagnostics();
            if( response->status == HTTP_GATEWAY_TIMEOUT ) {
                const auto body = nlohmann::json::parse( response->body );
                EXPECT_EQ( body.at( "error" ), "gimbal_timeout" );
                EXPECT_EQ( body.at( "outcome" ), "unknown" );
            }
            // GIMBAL_REQUEST_TIMEOUT_MS is documented at 1000 ms; this is a generous upper bound,
            // not an attempt to pin the exact timeout value.
            EXPECT_LT( elapsed, std::chrono::seconds( 5 ) ) << "an unavailable gimbal must fail bounded, not hang";

            ASSERT_TRUE( harness.Gateway_Alive() ) << "gateway must survive an unreachable command peer";
            ASSERT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) )
                << "gateway must still answer readiness after an unavailable-peer slew;" << harness.Diagnostics();
        }

        /** @test Verifies that malformed protobuf telemetry is rejected then valid message recovers. */
        TEST_F( Negative_Paths_Test, Malformed_Protobuf_Telemetry_Is_Rejected_Then_Valid_Message_Recovers ) {
            // No dummy_monolith process at all: the harness itself binds a raw PUB socket on the
            // configured telemetry endpoint and acts as a rogue publisher, exactly mirroring
            // services/api_gateway/test/telemetry_subscriber_test.cpp's unit-level garbage-payload
            // case but through the real api_gateway executable and its real REST surface.
            harness.Start_Gateway();
            ASSERT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();

            edge::ipc::Context context;
            auto publisher_result = edge::ipc::Publisher::Bind( context, harness.Telemetry_Endpoint() );
            ASSERT_TRUE( publisher_result.has_value() ) << "failed to bind rogue telemetry publisher";
            auto& publisher = *publisher_result;

            // A truncated varint (continuation bit set on every byte) is guaranteed to fail protobuf
            // parsing, unlike arbitrary short byte strings which can occasionally happen to parse as
            // a valid (if nonsensical) message.
            const std::string garbage_payload( 16, '\xFF' );

            const auto reject_deadline = std::chrono::steady_clock::now() + TELEMETRY_OBSERVATION_TIMEOUT;
            ASSERT_TRUE(
                support::Poll_Until( reject_deadline,
                                     std::chrono::milliseconds( 20 ),
                                     [&] {
                                         const auto publish_result = publisher.Publish( "cues", garbage_payload );
                                         return publish_result.has_value() &&
                                                harness.Gateway_Output_Contains( "event=telemetry_parse_failed" );
                                     } ) )
                << "gateway never reported the malformed telemetry payload;" << harness.Diagnostics();

            ASSERT_TRUE( harness.Gateway_Alive() ) << "gateway must survive malformed protobuf telemetry";

            const auto cues_after_garbage = harness.Get( "/api/v1/cues" );
            ASSERT_TRUE( cues_after_garbage );
            const auto cues_after_garbage_body = nlohmann::json::parse( cues_after_garbage->body );
            EXPECT_TRUE( cues_after_garbage_body.at( "targets" ).empty() )
                << "the cache must stay empty after only garbage was ever published";

            edge::telemetry::v1::CuePriorityList valid_list;
            auto* target = valid_list.add_targets();
            target->set_uid( "drone_01" );
            target->set_latitude_deg( SAMPLE_LATITUDE_DEG );
            target->set_longitude_deg( SAMPLE_LONGITUDE_DEG );
            target->set_altitude_m( SAMPLE_ALTITUDE_M );
            std::string valid_bytes;
            ASSERT_TRUE( valid_list.SerializeToString( &valid_bytes ) );

            constexpr std::array<std::string_view, 1> EXPECTED_UIDS { "drone_01" };
            const auto ingest_deadline = std::chrono::steady_clock::now() + TELEMETRY_OBSERVATION_TIMEOUT;
            ASSERT_TRUE( support::Poll_Until( ingest_deadline,
                                              std::chrono::milliseconds( 20 ),
                                              [&] {
                                                  const auto publish_result = publisher.Publish( "cues", valid_bytes );
                                                  return publish_result.has_value() &&
                                                         harness.Cues_Match( EXPECTED_UIDS );
                                              } ) )
                << "a subsequent valid publication was never observable through /api/v1/cues;" << harness.Diagnostics();

            const auto cues_after_valid = harness.Get( "/api/v1/cues" );
            ASSERT_TRUE( cues_after_valid );
            const auto cues_after_valid_body = nlohmann::json::parse( cues_after_valid->body );
            const auto& targets = cues_after_valid_body.at( "targets" );
            ASSERT_EQ( targets.size(), 1U );
            EXPECT_EQ( targets.at( 0 ).at( "uid" ).get<std::string>(), "drone_01" );
        }

    }  // namespace
}  // namespace edge::integration_test
