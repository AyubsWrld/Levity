/**
 * @file zmq_command_server_test.cpp
 * @brief Test implementation for zmq command server test behavior and regression coverage.
 *
 * @details Source path: `services/gimbal_controller/test/adapters/inbound/commands/zmq_command_server_test.cpp`.
 */

#include "commands.pb.h"

#include <edge/gimbal_controller/adapters/inbound/commands/zmq_command_server.h>
#include <edge/gimbal_controller/core/ports/inbound/command_port.h>
#include <edge/ipc/context.h>
#include <edge/ipc/error.h>
#include <edge/ipc/requester.h>
#include <edge/platform/logging/logger.h>
#include <edge/result.h>
#include <edge/test/temp_dir.h>
#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>

namespace edge::gimbal_controller::adapters::inbound::commands {
    namespace {

        constexpr auto TEST_DEADLINE = std::chrono::seconds( 5 );
        constexpr auto REQUEST_TIMEOUT = std::chrono::milliseconds( 500 );
        /** Sample target coordinates used by the minimal valid SlewCommand fixture. */
        constexpr double SAMPLE_LATITUDE_DEG = 34.05;
        constexpr double SAMPLE_LONGITUDE_DEG = -118.25;
        constexpr double SAMPLE_ALTITUDE_M = 150.0;

        /** @brief Inbound-port fake recording calls so the ZeroMQ command adapter can be tested in isolation. */
        class Fake_Command_Port final : public core::ports::inbound::Command_Port {
        public:
            /** @brief Records the command and returns the configured fake domain reply. */
            [[nodiscard]] auto Handle_Slew_Command( const edge::commands::v1::SlewCommand& command )
                -> edge::commands::v1::SlewCommandReply override {
                ++m_call_count;
                m_last_target_uid = command.target_uid();

                edge::commands::v1::SlewCommandReply reply;
                reply.set_target_uid( command.target_uid() );
                reply.set_status( edge::commands::v1::SLEW_STATUS_ACCEPTED );
                return reply;
            }

            /** @brief Returns how many commands the adapter forwarded to the fake core port. */
            [[nodiscard]] auto Call_Count() const noexcept -> int { return m_call_count; }
            /** @brief Returns the UID observed in the most recently forwarded command. */
            [[nodiscard]] auto Last_Target_Uid() const noexcept -> const std::string& { return m_last_target_uid; }

        private:
            int m_call_count = 0;
            std::string m_last_target_uid;
        };

        /** @brief Issues one request using the real IPC wrapper with a bounded test deadline. */
        [[nodiscard]] auto Request_With_Deadline( edge::ipc::Requester& requester, std::string_view payload )
            -> edge::Result<std::string, edge::ipc::Error> {
            edge::Result<std::string, edge::ipc::Error> result =
                edge::Make_Failure( edge::ipc::Error { edge::ipc::Error_Code::TIMEOUT, "not attempted" } );

            const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
            while( std::chrono::steady_clock::now() < deadline ) {
                result = requester.Request( payload, REQUEST_TIMEOUT );
                if( result.has_value() ) {
                    return result;
                }
            }
            return result;
        }

        /** @brief Builds a minimal valid SlewCommand fixture. */
        [[nodiscard]] auto Make_Command( std::string uid ) -> edge::commands::v1::SlewCommand {
            edge::commands::v1::SlewCommand command;
            command.set_target_uid( uid );
            auto* target = command.mutable_target();
            target->set_uid( std::move( uid ) );
            target->set_latitude_deg( SAMPLE_LATITUDE_DEG );
            target->set_longitude_deg( SAMPLE_LONGITUDE_DEG );
            target->set_altitude_m( SAMPLE_ALTITUDE_M );
            return command;
        }

        /** @test Verifies that valid command is forwarded through inbound port. */
        TEST( Zmq_Command_Server_Test, Valid_Command_Is_Forwarded_Through_Inbound_Port ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/cmd.ipc";

            edge::ipc::Context context;
            Fake_Command_Port commands;
            edge::platform::logging::Logger logger { "gimbal_controller_test" };
            const Zmq_Command_Server server( context, endpoint, commands, logger );

            edge::ipc::Context requester_context;
            auto requester_result = edge::ipc::Requester::Connect( requester_context, endpoint );
            ASSERT_TRUE( requester_result.has_value() );

            std::string serialized_command;
            ASSERT_TRUE( Make_Command( "drone_01" ).SerializeToString( &serialized_command ) );

            const auto response = Request_With_Deadline( *requester_result, serialized_command );
            ASSERT_TRUE( response.has_value() );

            edge::commands::v1::SlewCommandReply reply;
            ASSERT_TRUE( reply.ParseFromString( *response ) );
            EXPECT_EQ( reply.status(), edge::commands::v1::SLEW_STATUS_ACCEPTED );
            EXPECT_EQ( commands.Call_Count(), 1 );
            EXPECT_EQ( commands.Last_Target_Uid(), "drone_01" );
        }

        /** @test Verifies that malformed payload is rejected without calling core and server remains usable. */
        TEST( Zmq_Command_Server_Test, Malformed_Payload_Is_Rejected_Without_Calling_Core_And_Server_Remains_Usable ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/cmd.ipc";

            edge::ipc::Context context;
            Fake_Command_Port commands;
            edge::platform::logging::Logger logger { "gimbal_controller_test" };
            const Zmq_Command_Server server( context, endpoint, commands, logger );

            edge::ipc::Context requester_context;
            auto requester_result = edge::ipc::Requester::Connect( requester_context, endpoint );
            ASSERT_TRUE( requester_result.has_value() );

            const std::string garbage_payload( "\xFF\xFE\xFD\xFC\xFB\xFA\xF9\xF8", 8 );
            const auto malformed_response = Request_With_Deadline( *requester_result, garbage_payload );
            ASSERT_TRUE( malformed_response.has_value() );

            edge::commands::v1::SlewCommandReply malformed_reply;
            ASSERT_TRUE( malformed_reply.ParseFromString( *malformed_response ) );
            EXPECT_EQ( malformed_reply.status(), edge::commands::v1::SLEW_STATUS_REJECTED_INVALID );
            EXPECT_EQ( commands.Call_Count(), 0 );

            std::string serialized_command;
            ASSERT_TRUE( Make_Command( "drone_02" ).SerializeToString( &serialized_command ) );
            const auto valid_response = Request_With_Deadline( *requester_result, serialized_command );
            ASSERT_TRUE( valid_response.has_value() );

            edge::commands::v1::SlewCommandReply valid_reply;
            ASSERT_TRUE( valid_reply.ParseFromString( *valid_response ) );
            EXPECT_EQ( valid_reply.status(), edge::commands::v1::SLEW_STATUS_ACCEPTED );
            EXPECT_EQ( commands.Call_Count(), 1 );
        }

        /** @test Verifies that invalid bind path is startup failure. */
        TEST( Zmq_Command_Server_Test, Invalid_Bind_Path_Is_Startup_Failure ) {
            edge::ipc::Context context;
            Fake_Command_Port commands;
            edge::platform::logging::Logger logger { "gimbal_controller_test" };

            EXPECT_THROW( ( Zmq_Command_Server { context, "ipc:///proc/edge-test/socket.ipc", commands, logger } ),
                          std::runtime_error );
        }

    }  // namespace
}  // namespace edge::gimbal_controller::adapters::inbound::commands
