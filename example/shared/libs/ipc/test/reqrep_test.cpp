/**
 * @file reqrep_test.cpp
 * @brief Test implementation for reqrep test behavior and regression coverage.
 *
 * @details Source path: `shared/libs/ipc/test/reqrep_test.cpp`.
 */

#include "edge/ipc/context.h"
#include "edge/ipc/error.h"
#include "edge/ipc/limits.h"
#include "edge/ipc/replier.h"
#include "edge/ipc/requester.h"

#include <edge/result.h>
#include <edge/test/temp_dir.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <optional>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>

namespace edge::ipc {
    namespace {

        constexpr auto TEST_DEADLINE = std::chrono::seconds( 5 );
        /** Negative timeout used to prove a non-blocking poll normalization. */
        constexpr auto NEGATIVE_TIMEOUT = std::chrono::milliseconds( -1 );
        /** Short poll interval used while waiting for a Pending_Request in a loop. */
        constexpr auto POLL_TIMEOUT = std::chrono::milliseconds( 50 );
        /** Timeout for the oversized-payload rejection request, which fails before any I/O. */
        constexpr auto OVERSIZED_SEND_TIMEOUT = std::chrono::milliseconds( 100 );
        /** Timeout used for ordinary request/reply round trips in these tests. */
        constexpr auto REQUEST_TIMEOUT = std::chrono::milliseconds( 200 );
        /** Generous upper bound used to detect an accidental infinite libzmq timeout. */
        constexpr auto NONBLOCKING_TIMING_BOUND = std::chrono::milliseconds( 500 );
        /** Timeout for a request that is deliberately left unanswered by the Replier. */
        constexpr auto UNANSWERED_REQUEST_TIMEOUT = std::chrono::milliseconds( 500 );
        /** Timeout for a request that outlives the Replier move-rejection assertions. */
        constexpr auto PENDING_REQUEST_TIMEOUT = std::chrono::milliseconds( 1000 );

        /** @test Verifies that rejects malformed endpoint. */
        TEST( Requester_Create_Test, Rejects_Malformed_Endpoint ) {
            Context context;

            const auto result = Requester::Connect( context, "not-a-valid-endpoint" );

            ASSERT_FALSE( result.has_value() );
            EXPECT_EQ( result.error().code, Error_Code::INVALID_ENDPOINT );
        }

        /** @test Verifies that rejects malformed endpoint. */
        TEST( Replier_Create_Test, Rejects_Malformed_Endpoint ) {
            Context context;

            const auto result = Replier::Bind( context, "not-a-valid-endpoint" );

            ASSERT_FALSE( result.has_value() );
            EXPECT_EQ( result.error().code, Error_Code::INVALID_ENDPOINT );
        }

        /** @test Verifies that rejects oversized payload before send. */
        TEST( Requester_Test, Rejects_Oversized_Payload_Before_Send ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/cmd.ipc";

            Context context;
            auto requester_result = Requester::Connect( context, endpoint );
            ASSERT_TRUE( requester_result.has_value() );

            const std::string oversized( MAX_SERIALIZED_MESSAGE_BYTES + 1, 'a' );
            const auto result = requester_result->Request( oversized, OVERSIZED_SEND_TIMEOUT );

            ASSERT_FALSE( result.has_value() );
            EXPECT_EQ( result.error().code, Error_Code::MESSAGE_TOO_LARGE );
        }

        /** @test Verifies that a negative receive timeout is normalized to a non-blocking poll. */
        TEST( Replier_Test, Negative_Receive_Timeout_Is_Nonblocking ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/cmd.ipc";

            Context context;
            auto replier_result = Replier::Bind( context, endpoint );
            ASSERT_TRUE( replier_result.has_value() );

            const auto start = std::chrono::steady_clock::now();
            const auto receive_result = replier_result->Receive( NEGATIVE_TIMEOUT );
            const auto elapsed = std::chrono::steady_clock::now() - start;

            ASSERT_TRUE( receive_result.has_value() );
            EXPECT_FALSE( receive_result->has_value() );
            // This generous bound detects an accidental libzmq infinite timeout without depending on
            // scheduler-level timing precision.
            EXPECT_LT( elapsed, NONBLOCKING_TIMING_BOUND );
        }

        /** @test Verifies that receive times out returns nullopt. */
        TEST( Replier_Test, Receive_Times_Out_Returns_Nullopt ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/cmd.ipc";

            Context context;
            auto replier_result = Replier::Bind( context, endpoint );
            ASSERT_TRUE( replier_result.has_value() );

            const auto receive_result = replier_result->Receive( POLL_TIMEOUT );

            ASSERT_TRUE( receive_result.has_value() );
            EXPECT_FALSE( receive_result->has_value() );
        }

        /** @test Verifies that round trip. */
        TEST( Req_Rep_Test, Round_Trip ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/cmd.ipc";

            std::string observed_payload;

            // The Replier is created and used entirely on its own thread: a socket is created, used and
            // destroyed on exactly one thread (see edge/ipc/replier.h).
            std::jthread replier_thread( [&]( const std::stop_token& stop_token ) {
                Context replier_context;
                auto replier_result = Replier::Bind( replier_context, endpoint );
                if( !replier_result.has_value() ) {
                    return;
                }
                auto& replier = *replier_result;

                const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
                while( !stop_token.stop_requested() && std::chrono::steady_clock::now() < deadline ) {
                    auto receive_result = replier.Receive( POLL_TIMEOUT );
                    if( !receive_result.has_value() || !receive_result->has_value() ) {
                        continue;
                    }

                    observed_payload.assign( ( *receive_result )->Payload() );
                    std::ignore = ( *receive_result )->Reply( "reply-payload" );
                    return;
                }
            } );

            Context requester_context;
            auto requester_result = Requester::Connect( requester_context, endpoint );
            ASSERT_TRUE( requester_result.has_value() );
            auto& requester = *requester_result;

            edge::Result<std::string, Error> final_result =
                edge::Make_Failure( Error { Error_Code::TIMEOUT, "not attempted" } );
            const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
            while( std::chrono::steady_clock::now() < deadline ) {
                final_result = requester.Request( "request-payload", REQUEST_TIMEOUT );
                if( final_result.has_value() ) {
                    break;
                }
            }

            replier_thread.request_stop();
            replier_thread.join();

            ASSERT_TRUE( final_result.has_value() );
            EXPECT_EQ( *final_result, "reply-payload" );
            EXPECT_EQ( observed_payload, "request-payload" );
        }

        /** @test Verifies that timeout when no peer then recovers after peer binds. */
        TEST( Requester_Test, Timeout_When_No_Peer_Then_Recovers_After_Peer_Binds ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/cmd.ipc";

            Context requester_context;
            auto requester_result = Requester::Connect( requester_context, endpoint );
            ASSERT_TRUE( requester_result.has_value() );
            auto& requester = *requester_result;

            // No Replier is bound yet: the request must time out (ambiguous outcome, never retried by
            // this class).
            const auto timeout_result = requester.Request( "request-1", REQUEST_TIMEOUT );

            ASSERT_FALSE( timeout_result.has_value() );
            EXPECT_EQ( timeout_result.error().code, Error_Code::TIMEOUT );

            std::string observed_payload;
            std::jthread replier_thread( [&]( const std::stop_token& stop_token ) {
                Context replier_context;
                auto replier_result = Replier::Bind( replier_context, endpoint );
                if( !replier_result.has_value() ) {
                    return;
                }
                auto& replier = *replier_result;

                const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
                while( !stop_token.stop_requested() && std::chrono::steady_clock::now() < deadline ) {
                    auto receive_result = replier.Receive( POLL_TIMEOUT );
                    if( !receive_result.has_value() || !receive_result->has_value() ) {
                        continue;
                    }

                    observed_payload.assign( ( *receive_result )->Payload() );
                    std::ignore = ( *receive_result )->Reply( "ack" );
                    return;
                }
            } );

            // Proves socket recreation: the *same* Requester instance is used again after the earlier
            // timeout, and this time it must succeed once a real peer is bound.
            edge::Result<std::string, Error> final_result =
                edge::Make_Failure( Error { Error_Code::TIMEOUT, "not attempted" } );
            const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
            while( std::chrono::steady_clock::now() < deadline ) {
                final_result = requester.Request( "request-2", REQUEST_TIMEOUT );
                if( final_result.has_value() ) {
                    break;
                }
            }

            replier_thread.request_stop();
            replier_thread.join();

            ASSERT_TRUE( final_result.has_value() );
            EXPECT_EQ( *final_result, "ack" );
            EXPECT_EQ( observed_payload, "request-2" );
        }

        /** @test Verifies that second receive without reply returns protocol state error. */
        TEST( Replier_Test, Second_Receive_Without_Reply_Returns_Protocol_State_Error ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/cmd.ipc";

            Context context;
            auto replier_result = Replier::Bind( context, endpoint );
            ASSERT_TRUE( replier_result.has_value() );
            auto& replier = *replier_result;

            std::jthread requester_thread( [&] {
                Context requester_context;
                auto requester_result = Requester::Connect( requester_context, endpoint );
                if( !requester_result.has_value() ) {
                    return;
                }
                std::ignore = requester_result->Request( "request-payload", UNANSWERED_REQUEST_TIMEOUT );
            } );

            std::optional<Pending_Request> pending;
            const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
            while( !pending.has_value() && std::chrono::steady_clock::now() < deadline ) {
                auto receive_result = replier.Receive( POLL_TIMEOUT );
                ASSERT_TRUE( receive_result.has_value() );
                pending.emplace( std::move( **receive_result ) );
            }
            ASSERT_TRUE( pending.has_value() );
            // The optional is checked immediately above; suppress the conservative unchecked-
            // optional-access diagnostic around the GTest control-flow macro expansion.
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            EXPECT_EQ( pending.value().Payload(), "request-payload" );

            // Deliberately never call pending->Reply(): the Replier must now refuse to receive again.
            const auto second_receive_result = replier.Receive( POLL_TIMEOUT );

            ASSERT_FALSE( second_receive_result.has_value() );
            EXPECT_EQ( second_receive_result.error().code, Error_Code::PROTOCOL_STATE_ERROR );

            requester_thread.join();
        }

        /** @test Verifies a live Pending_Request prevents moving its owning Replier. */
        TEST( Replier_Test, Move_With_Pending_Request_Is_Rejected ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/cmd.ipc";

            Context replier_context;
            auto replier_result = Replier::Bind( replier_context, endpoint );
            ASSERT_TRUE( replier_result.has_value() );
            auto& replier = *replier_result;

            std::promise<edge::Result<std::string, Error>> reply_promise;
            auto reply_future = reply_promise.get_future();
            std::jthread requester_thread( [&]( const std::stop_token& ) {
                Context requester_context;
                auto requester_result = Requester::Connect( requester_context, endpoint );
                if( !requester_result.has_value() ) {
                    reply_promise.set_value( edge::Make_Failure( requester_result.error() ) );
                    return;
                }
                reply_promise.set_value( requester_result->Request( "command", PENDING_REQUEST_TIMEOUT ) );
            } );

            std::optional<Pending_Request> pending;
            const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
            while( !pending.has_value() && std::chrono::steady_clock::now() < deadline ) {
                auto receive_result = replier.Receive( POLL_TIMEOUT );
                ASSERT_TRUE( receive_result.has_value() );
                if( receive_result->has_value() ) {
                    pending.emplace( std::move( **receive_result ) );
                }
            }
            ASSERT_TRUE( pending.has_value() );

            EXPECT_THROW( [[maybe_unused]] auto moved = std::move( replier ), std::logic_error );
            ASSERT_TRUE( pending->Reply( "reply" ).has_value() );
            requester_thread.join();
            ASSERT_TRUE( reply_future.get().has_value() );
        }

    }  // namespace
}  // namespace edge::ipc
