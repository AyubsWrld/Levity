/**
 * @file zmq_gimbal_command_client_test.cpp
 * @brief Test implementation for zmq gimbal command client test behavior and regression coverage.
 *
 * @details Source path: `services/api_gateway/test/adapters/outbound/gimbal/zmq_gimbal_command_client_test.cpp`.
 */

#include "commands.pb.h"

#include <edge/api_gateway/adapters/outbound/gimbal/zmq_gimbal_command_client.h>
#include <edge/api_gateway/core/ports/outbound/gimbal_command_port.h>
#include <edge/ipc/context.h>
#include <edge/ipc/replier.h>
#include <edge/result.h>
#include <edge/test/temp_dir.h>
#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <stop_token>
#include <string>
#include <thread>
#include <tuple>

namespace edge::api_gateway::adapters::outbound::gimbal {
    namespace {

        constexpr auto TEST_DEADLINE = std::chrono::seconds( 5 );
        /** Poll interval used by the fake gimbal REP peer while waiting for a request. */
        constexpr auto REPLIER_POLL_TIMEOUT = std::chrono::milliseconds( 50 );
        /** Sample target coordinates used by the domain command fixture. */
        constexpr double SAMPLE_LATITUDE_DEG = 1.0;
        constexpr double SAMPLE_LONGITUDE_DEG = 2.0;
        constexpr double SAMPLE_ALTITUDE_M = 3.0;

        /** @brief Builds a minimal domain slew command used by command-client tests. */
        [[nodiscard]] auto Make_Command( std::string_view uid ) -> edge::commands::v1::SlewCommand {
            edge::commands::v1::SlewCommand command;
            command.set_target_uid( std::string( uid ) );
            command.mutable_target()->set_uid( std::string( uid ) );
            command.mutable_target()->set_latitude_deg( SAMPLE_LATITUDE_DEG );
            command.mutable_target()->set_longitude_deg( SAMPLE_LONGITUDE_DEG );
            command.mutable_target()->set_altitude_m( SAMPLE_ALTITUDE_M );
            return command;
        }

        /** @brief Serializes a successful fake gimbal reply for a given target UID. */
        [[nodiscard]] auto Serialize_Accepted_Reply( std::string_view uid ) -> std::string {
            edge::commands::v1::SlewCommandReply reply;
            reply.set_status( edge::commands::v1::SLEW_STATUS_ACCEPTED );
            reply.set_target_uid( std::string( uid ) );
            reply.set_detail( "ok" );

            std::string bytes;
            reply.SerializeToString( &bytes );
            return bytes;
        }

        // Runs a fake gimbal REP peer until it has replied to exactly one request (with
        // `reply_bytes`) or `stop_token` is signalled / `TEST_DEADLINE` elapses.
        /** @brief Runs a bounded fake REP peer used to exercise the real outbound ZeroMQ adapter. */
        void Run_Fake_Gimbal( const std::stop_token& stop_token,
                              edge::ipc::Context& context,
                              const std::string& endpoint,
                              const std::string& reply_bytes ) {
            auto replier_result = edge::ipc::Replier::Bind( context, endpoint );
            if( !replier_result.has_value() ) {
                return;
            }
            auto& replier = *replier_result;

            const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
            while( !stop_token.stop_requested() && std::chrono::steady_clock::now() < deadline ) {
                auto receive_result = replier.Receive( REPLIER_POLL_TIMEOUT );
                if( !receive_result.has_value() || !receive_result->has_value() ) {
                    continue;
                }

                // The optional is checked immediately above; suppress the conservative unchecked-
                // optional-access diagnostic around the control-flow guard's macro-free branch.
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                std::ignore = ( *receive_result )->Reply( reply_bytes );
                return;
            }
        }

        /** @test Verifies that round trip returns reply from fake gimbal. */
        TEST( Zmq_Gimbal_Command_Client_Test, Round_Trip_Returns_Reply_From_Fake_Gimbal ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/cmd.ipc";

            edge::ipc::Context context;

            std::jthread gimbal_thread( [&]( const std::stop_token& stop_token ) {
                Run_Fake_Gimbal( stop_token, context, endpoint, Serialize_Accepted_Reply( "drone_01" ) );
            } );

            Zmq_Gimbal_Command_Client client( context, endpoint );
            const auto command = Make_Command( "drone_01" );

            edge::Result<edge::commands::v1::SlewCommandReply, core::ports::outbound::Gimbal_Command_Error> result =
                edge::Make_Failure( core::ports::outbound::Gimbal_Command_Error {
                    core::ports::outbound::Gimbal_Command_Error_Code::TIMEOUT, "not attempted" } );
            const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
            while( !result.has_value() && std::chrono::steady_clock::now() < deadline ) {
                result = client.Send_Slew( command );
            }

            gimbal_thread.request_stop();
            gimbal_thread.join();

            ASSERT_TRUE( result.has_value() );
            EXPECT_EQ( result->status(), edge::commands::v1::SLEW_STATUS_ACCEPTED );
            EXPECT_EQ( result->target_uid(), "drone_01" );
        }

        /** @test Verifies that no peer bound returns timeout then recovers after peer binds. */
        TEST( Zmq_Gimbal_Command_Client_Test, No_Peer_Bound_Returns_Timeout_Then_Recovers_After_Peer_Binds ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/cmd.ipc";

            edge::ipc::Context context;
            Zmq_Gimbal_Command_Client client( context, endpoint );
            const auto command = Make_Command( "drone_01" );

            // No REP peer is bound at all yet: the request must resolve to TIMEOUT (ambiguous) within
            // a bounded deadline rather than hanging.
            const auto timeout_result = client.Send_Slew( command );

            ASSERT_FALSE( timeout_result.has_value() );
            EXPECT_EQ( timeout_result.error().code, core::ports::outbound::Gimbal_Command_Error_Code::TIMEOUT );

            // The same client must remain usable for a following successful request once a peer binds.
            std::jthread gimbal_thread( [&]( const std::stop_token& stop_token ) {
                Run_Fake_Gimbal( stop_token, context, endpoint, Serialize_Accepted_Reply( "drone_01" ) );
            } );

            edge::Result<edge::commands::v1::SlewCommandReply, core::ports::outbound::Gimbal_Command_Error>
                final_result = edge::Make_Failure( core::ports::outbound::Gimbal_Command_Error {
                    core::ports::outbound::Gimbal_Command_Error_Code::TIMEOUT, "not attempted" } );
            const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
            while( !final_result.has_value() && std::chrono::steady_clock::now() < deadline ) {
                final_result = client.Send_Slew( command );
            }

            gimbal_thread.request_stop();
            gimbal_thread.join();

            ASSERT_TRUE( final_result.has_value() );
            EXPECT_EQ( final_result->status(), edge::commands::v1::SLEW_STATUS_ACCEPTED );
        }

        /** @test Verifies that stop while request pending satisfies promise with error. */
        TEST( Zmq_Gimbal_Command_Client_Test, Stop_While_Request_Pending_Satisfies_Promise_With_Error ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/cmd.ipc";

            edge::ipc::Context context;
            Zmq_Gimbal_Command_Client client( context, endpoint );  // no REP peer ever bound

            const auto command = Make_Command( "drone_01" );

            std::promise<
                edge::Result<edge::commands::v1::SlewCommandReply, core::ports::outbound::Gimbal_Command_Error>>
                result_promise;
            auto result_future = result_promise.get_future();

            std::jthread caller_thread( [&] { result_promise.set_value( client.Send_Slew( command ) ); } );

            // Request shutdown while the call above may still be queued or in-flight. Either ordering
            // (rejected immediately as shutting-down, or resolved later by the draining worker) must
            // produce an error result within a bounded deadline - never a hang.
            client.Stop();

            const auto wait_status = result_future.wait_for( TEST_DEADLINE );
            ASSERT_EQ( wait_status, std::future_status::ready );

            caller_thread.join();

            const auto result = result_future.get();
            EXPECT_FALSE( result.has_value() );
        }

    }  // namespace
}  // namespace edge::api_gateway::adapters::outbound::gimbal
