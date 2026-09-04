/**
 * @file slew_to_cue_test.cpp
 * @brief Test implementation for slew to cue test behavior and regression coverage.
 *
 * @details Source path: `test/integration/slew_to_cue_test.cpp`.
 */

#include "deterministic_targets.h"
#include "stanag_wire.h"
#include "test_harness.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstddef>
#include <string>

// Tier 1 happy-path acceptance coverage: telemetry -> REST -> slew -> ZeroMQ -> UDP
// (.github/bootstrap-mvp.md Phase 6; docs/architecture/tier1-contracts.md §9/§10).
namespace edge::integration_test {
    namespace {

        constexpr auto READY_TIMEOUT = std::chrono::seconds( 10 );
        constexpr auto TELEMETRY_TIMEOUT = std::chrono::seconds( 10 );
        constexpr auto UDP_TIMEOUT = std::chrono::seconds( 3 );

        /** @brief End-to-end fixture that starts the full Tier-1 flow and captures gimbal UDP output. */
        class Slew_To_Cue_Test : public ::testing::Test {
        protected:
            // GTest fixtures conventionally expose state via `protected` so that TEST_F-generated
            // subclasses can access it directly; there is no external caller to encapsulate against.
            // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
            support::Test_Harness harness;

            /** @brief Starts all three processes and waits for deterministic telemetry readiness. */
            void SetUp() override {
                // Startup order matches the recommended sequence: gimbal (bind) first, then gateway
                // (connects both ways), then the monolith (PUB bind) last - so the gateway's SUB is
                // already connecting by the time telemetry starts flowing.
                harness.Start_Gimbal();
                ASSERT_TRUE( harness.Wait_For_Gimbal_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();

                harness.Start_Gateway();
                ASSERT_TRUE( harness.Wait_For_Gateway_Ready( READY_TIMEOUT ) ) << harness.Diagnostics();

                harness.Start_Monolith();
                ASSERT_TRUE( harness.Wait_For_Deterministic_Telemetry( TELEMETRY_TIMEOUT ) ) << harness.Diagnostics();
            }
        };

        /** @test Verifies that cues report deterministic targets in published order. */
        TEST_F( Slew_To_Cue_Test, Cues_Report_Deterministic_Targets_In_Published_Order ) {
            const auto response = harness.Get( "/api/v1/cues" );
            ASSERT_TRUE( response ) << harness.Diagnostics();
            ASSERT_EQ( response->status, 200 ) << harness.Diagnostics();

            const auto body = nlohmann::json::parse( response->body );
            const auto& targets = body.at( "targets" );
            ASSERT_EQ( targets.size(), support::DETERMINISTIC_TARGETS.size() );

            for( std::size_t index = 0; index < support::DETERMINISTIC_TARGETS.size(); ++index ) {
                const auto& expected = support::DETERMINISTIC_TARGETS.at( index );
                const auto& actual = targets.at( index );
                EXPECT_EQ( actual.at( "uid" ).get<std::string>(), expected.uid ) << "index=" << index;
                EXPECT_DOUBLE_EQ( actual.at( "latitude_deg" ).get<double>(), expected.latitude_deg )
                    << "index=" << index;
                EXPECT_DOUBLE_EQ( actual.at( "longitude_deg" ).get<double>(), expected.longitude_deg )
                    << "index=" << index;
                EXPECT_DOUBLE_EQ( actual.at( "altitude_m" ).get<double>(), expected.altitude_m ) << "index=" << index;
            }
        }

        /** @test Verifies that accepted slew emits exact stanag datagram pair. */
        TEST_F( Slew_To_Cue_Test, Accepted_Slew_Emits_Exact_Stanag_Datagram_Pair ) {
            const auto response = harness.Post( "/api/v1/slew", R"({"target_uid":"drone_01"})" );
            ASSERT_TRUE( response ) << harness.Diagnostics();
            ASSERT_EQ( response->status, 200 ) << harness.Diagnostics();

            const auto body = nlohmann::json::parse( response->body );
            EXPECT_EQ( body.at( "status" ), "accepted" );
            EXPECT_EQ( body.at( "target_uid" ), "drone_01" );

            const auto datagram_200 = harness.Udp().Receive_Datagram( UDP_TIMEOUT );
            ASSERT_TRUE( datagram_200.has_value() ) << "no #200 datagram received;" << harness.Diagnostics();
            // The optional is checked immediately above; suppress the conservative unchecked-
            // optional-access diagnostic around the GTest control-flow macro expansion.
            // NOLINTBEGIN(bugprone-unchecked-optional-access)
            ASSERT_EQ( datagram_200->size(), support::STEERING_COMMAND_DATAGRAM_SIZE );

            const auto decoded = support::Decode_Steering_Command( *datagram_200 );
            EXPECT_EQ( decoded.message_id, support::STEERING_COMMAND_MESSAGE_ID );
            EXPECT_EQ( decoded.message_length, support::STEERING_COMMAND_DATAGRAM_SIZE );
            EXPECT_EQ( decoded.sequence, 1U );
            EXPECT_DOUBLE_EQ( decoded.latitude_deg, support::DETERMINISTIC_TARGETS.at( 0 ).latitude_deg );
            EXPECT_DOUBLE_EQ( decoded.longitude_deg, support::DETERMINISTIC_TARGETS.at( 0 ).longitude_deg );
            EXPECT_DOUBLE_EQ( decoded.altitude_m, support::DETERMINISTIC_TARGETS.at( 0 ).altitude_m );

            const auto expected_200 =
                support::Encode_Steering_Command( support::DETERMINISTIC_TARGETS.at( 0 ).latitude_deg,
                                                  support::DETERMINISTIC_TARGETS.at( 0 ).longitude_deg,
                                                  support::DETERMINISTIC_TARGETS.at( 0 ).altitude_m,
                                                  1 );
            EXPECT_EQ( *datagram_200, expected_200 ) << "message #200 is not byte-for-byte identical to the spec";
            // NOLINTEND(bugprone-unchecked-optional-access)

            const auto datagram_201 = harness.Udp().Receive_Datagram( UDP_TIMEOUT );
            ASSERT_TRUE( datagram_201.has_value() ) << "no #201 datagram received;" << harness.Diagnostics();
            const auto expected_201 = support::Encode_Steering_Mode( 1 );
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            EXPECT_EQ( *datagram_201, expected_201 ) << "message #201 is not byte-for-byte identical to the spec";

            // Exactly two datagrams per accepted slew: nothing further should arrive.
            const auto unexpected_third = harness.Udp().Receive_Datagram( std::chrono::milliseconds( 300 ) );
            EXPECT_FALSE( unexpected_third.has_value() ) << "unexpected third UDP datagram after one accepted slew";
        }

        /** @test Verifies that sequential slews increment sequence number. */
        TEST_F( Slew_To_Cue_Test, Sequential_Slews_Increment_Sequence_Number ) {
            const auto first_response = harness.Post( "/api/v1/slew", R"({"target_uid":"drone_01"})" );
            ASSERT_TRUE( first_response ) << harness.Diagnostics();
            ASSERT_EQ( first_response->status, 200 ) << harness.Diagnostics();

            const auto first_200 = harness.Udp().Receive_Datagram( UDP_TIMEOUT );
            ASSERT_TRUE( first_200.has_value() ) << harness.Diagnostics();
            const auto first_201 = harness.Udp().Receive_Datagram( UDP_TIMEOUT );
            ASSERT_TRUE( first_201.has_value() ) << harness.Diagnostics();
            // Both optionals are checked immediately above; suppress the conservative unchecked-
            // optional-access diagnostic around the GTest control-flow macro expansion.
            // NOLINTBEGIN(bugprone-unchecked-optional-access)
            EXPECT_EQ( support::Decode_Steering_Command( *first_200 ).sequence, 1U );
            EXPECT_EQ( *first_201, support::Encode_Steering_Mode( 1 ) );
            // NOLINTEND(bugprone-unchecked-optional-access)

            const auto second_response = harness.Post( "/api/v1/slew", R"({"target_uid":"drone_02"})" );
            ASSERT_TRUE( second_response ) << harness.Diagnostics();
            ASSERT_EQ( second_response->status, 200 ) << harness.Diagnostics();

            const auto second_200 = harness.Udp().Receive_Datagram( UDP_TIMEOUT );
            ASSERT_TRUE( second_200.has_value() ) << harness.Diagnostics();
            const auto second_201 = harness.Udp().Receive_Datagram( UDP_TIMEOUT );
            ASSERT_TRUE( second_201.has_value() ) << harness.Diagnostics();

            // NOLINTBEGIN(bugprone-unchecked-optional-access)
            const auto second_fields = support::Decode_Steering_Command( *second_200 );
            EXPECT_EQ( second_fields.sequence, 2U );
            EXPECT_DOUBLE_EQ( second_fields.latitude_deg, support::DETERMINISTIC_TARGETS.at( 1 ).latitude_deg );
            EXPECT_DOUBLE_EQ( second_fields.longitude_deg, support::DETERMINISTIC_TARGETS.at( 1 ).longitude_deg );
            EXPECT_DOUBLE_EQ( second_fields.altitude_m, support::DETERMINISTIC_TARGETS.at( 1 ).altitude_m );
            EXPECT_EQ( *second_201, support::Encode_Steering_Mode( 2 ) );
            // NOLINTEND(bugprone-unchecked-optional-access)
        }

    }  // namespace
}  // namespace edge::integration_test
