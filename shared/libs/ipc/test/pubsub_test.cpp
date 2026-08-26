/**
 * @file pubsub_test.cpp
 * @brief Test implementation for pubsub test behavior and regression coverage.
 *
 * @details Source path: `shared/libs/ipc/test/pubsub_test.cpp`.
 */

#include "edge/ipc/context.h"
#include "edge/ipc/error.h"
#include "edge/ipc/limits.h"
#include "edge/ipc/publisher.h"
#include "edge/ipc/subscriber.h"

#include <edge/test/temp_dir.h>
#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <string>

namespace edge::ipc {
    namespace {

        constexpr auto TEST_DEADLINE = std::chrono::seconds( 5 );
        /** Short poll interval used while waiting for a message in a retry loop. */
        constexpr auto POLL_TIMEOUT = std::chrono::milliseconds( 50 );

        /** @test Verifies that rejects malformed endpoint. */
        TEST( Publisher_Create_Test, Rejects_Malformed_Endpoint ) {
            Context context;

            const auto result = Publisher::Bind( context, "not-a-valid-endpoint" );

            ASSERT_FALSE( result.has_value() );
            EXPECT_EQ( result.error().code, Error_Code::INVALID_ENDPOINT );
        }

        /** @test Verifies that rejects malformed endpoint. */
        TEST( Subscriber_Create_Test, Rejects_Malformed_Endpoint ) {
            Context context;

            const auto result = Subscriber::Connect( context, "not-a-valid-endpoint", "cues" );

            ASSERT_FALSE( result.has_value() );
            EXPECT_EQ( result.error().code, Error_Code::INVALID_ENDPOINT );
        }

        /** @test Verifies that rejects oversized payload before send. */
        TEST( Publisher_Test, Rejects_Oversized_Payload_Before_Send ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/telemetry.ipc";

            Context context;
            auto publisher_result = Publisher::Bind( context, endpoint );
            ASSERT_TRUE( publisher_result.has_value() );

            const std::string oversized( MAX_SERIALIZED_MESSAGE_BYTES + 1, 'a' );
            const auto result = publisher_result->Publish( "cues", oversized );

            ASSERT_FALSE( result.has_value() );
            EXPECT_EQ( result.error().code, Error_Code::MESSAGE_TOO_LARGE );
        }

        /** @test Verifies that receive times out returns nullopt. */
        TEST( Subscriber_Test, Receive_Times_Out_Returns_Nullopt ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/telemetry.ipc";

            Context context;
            auto publisher_result = Publisher::Bind( context, endpoint );
            ASSERT_TRUE( publisher_result.has_value() );
            auto subscriber_result = Subscriber::Connect( context, endpoint, "cues" );
            ASSERT_TRUE( subscriber_result.has_value() );

            const auto receive_result = subscriber_result->Receive( POLL_TIMEOUT );

            ASSERT_TRUE( receive_result.has_value() );
            EXPECT_FALSE( receive_result->has_value() );
        }

        // Accounts for the PUB/SUB "slow joiner": a subscriber's connect/subscribe may not have propagated
        // to the publisher yet, so the first publication is not guaranteed to be observed. This retries
        // publishing with a bounded deadline instead of a fixed sleep.
        /** @test Verifies that round trip accounts for slow joiner. */
        TEST( Pub_Sub_Test, Round_Trip_Accounts_For_Slow_Joiner ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/telemetry.ipc";

            Context context;
            auto publisher_result = Publisher::Bind( context, endpoint );
            ASSERT_TRUE( publisher_result.has_value() );
            auto subscriber_result = Subscriber::Connect( context, endpoint, "cues" );
            ASSERT_TRUE( subscriber_result.has_value() );

            auto& publisher = *publisher_result;
            auto& subscriber = *subscriber_result;

            std::optional<Message> received;
            const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
            while( !received.has_value() && std::chrono::steady_clock::now() < deadline ) {
                const auto publish_result = publisher.Publish( "cues", "payload-bytes" );
                ASSERT_TRUE( publish_result.has_value() );

                auto receive_result = subscriber.Receive( POLL_TIMEOUT );
                ASSERT_TRUE( receive_result.has_value() );
                received = std::move( *receive_result );
            }

            ASSERT_TRUE( received.has_value() );
            // NOLINTBEGIN(bugprone-unchecked-optional-access) - confirmed clang-tidy false positive:
            // the checker cannot correlate GTest's ASSERT_TRUE macro expansion with `received` across
            // the preceding loop's reassignment, even though the value is definitely checked immediately
            // above on this exact object.
            EXPECT_EQ( received.value().topic, "cues" );
            EXPECT_EQ( received.value().payload, "payload-bytes" );
            // NOLINTEND(bugprone-unchecked-optional-access)
        }

        /** @test Verifies that filters out non matching topics. */
        TEST( Pub_Sub_Test, Filters_Out_Non_Matching_Topics ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/telemetry.ipc";

            Context context;
            auto publisher_result = Publisher::Bind( context, endpoint );
            ASSERT_TRUE( publisher_result.has_value() );
            auto subscriber_result = Subscriber::Connect( context, endpoint, "cues" );
            ASSERT_TRUE( subscriber_result.has_value() );

            auto& publisher = *publisher_result;
            auto& subscriber = *subscriber_result;

            // Establish the subscription (retry-publish the wanted topic until it is observed) before
            // asserting on filtering behaviour.
            std::optional<Message> received;
            const auto establish_deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
            while( !received.has_value() && std::chrono::steady_clock::now() < establish_deadline ) {
                const auto publish_result = publisher.Publish( "cues", "warm-up" );
                ASSERT_TRUE( publish_result.has_value() );

                auto receive_result = subscriber.Receive( POLL_TIMEOUT );
                ASSERT_TRUE( receive_result.has_value() );
                received = std::move( *receive_result );
            }
            ASSERT_TRUE( received.has_value() );

            // A non-matching topic must never be delivered: ZeroMQ applies the subscription filter itself,
            // so there is no slow-joiner-style race to account for here.
            const auto publish_other_result = publisher.Publish( "other-topic", "should-not-arrive" );
            ASSERT_TRUE( publish_other_result.has_value() );

            const auto receive_result = subscriber.Receive( std::chrono::milliseconds( 200 ) );

            ASSERT_TRUE( receive_result.has_value() );
            EXPECT_FALSE( receive_result->has_value() );
        }

        /** @test Verifies that a payload exactly at the configured maximum remains valid. */
        TEST( Pub_Sub_Test, Accepts_Payload_At_Maximum_Size ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/telemetry.ipc";

            Context context;
            auto publisher_result = Publisher::Bind( context, endpoint );
            ASSERT_TRUE( publisher_result.has_value() );
            auto subscriber_result = Subscriber::Connect( context, endpoint, "cues" );
            ASSERT_TRUE( subscriber_result.has_value() );

            auto& publisher = *publisher_result;
            auto& subscriber = *subscriber_result;

            // Publish::Publish() itself rejects an oversized payload before send (see
            // Publisher_Test.Rejects_Oversized_Payload_Before_Send), so a genuinely oversized frame can
            // only originate from a non-conforming peer. Exercise the receive-side guard directly by
            // publishing exactly at the limit and confirming it round-trips, establishing that the guard
            // is a strict "greater than" check consistent with the send-side guard.
            const std::string at_limit( MAX_SERIALIZED_MESSAGE_BYTES, 'a' );

            std::optional<Message> received;
            const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
            while( !received.has_value() && std::chrono::steady_clock::now() < deadline ) {
                const auto publish_result = publisher.Publish( "cues", at_limit );
                ASSERT_TRUE( publish_result.has_value() );

                auto receive_result = subscriber.Receive( POLL_TIMEOUT );
                ASSERT_TRUE( receive_result.has_value() );
                received = std::move( *receive_result );
            }

            ASSERT_TRUE( received.has_value() );
            // Confirmed clang-tidy false positive (see the identical justification in
            // Round_Trip_Accounts_For_Slow_Joiner above): the preceding loop's reassignment of
            // `received` defeats the checker's correlation of this ASSERT_TRUE with the
            // immediately-following access.
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            EXPECT_EQ( received.value().payload.size(), MAX_SERIALIZED_MESSAGE_BYTES );
        }

    }  // namespace
}  // namespace edge::ipc
