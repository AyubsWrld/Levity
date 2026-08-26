/**
 * @file rest_server_test.cpp
 * @brief Test implementation for rest server test behavior and regression coverage.
 *
 * @details Source path: `services/api_gateway/test/adapters/inbound/rest/rest_server_test.cpp`.
 */

#include "telemetry.pb.h"

#include <edge/api_gateway/adapters/inbound/rest/rest_server.h>
#include <edge/api_gateway/core/ports/inbound/api_port.h>
#include <edge/platform/logging/logger.h>
#include <edge/result.h>
#include <gtest/gtest.h>
#include <httplib/httplib.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace edge::api_gateway::adapters::inbound::rest {
    namespace {

        constexpr auto TEST_DEADLINE = std::chrono::seconds( 5 );
        /** HTTP status code used by the readiness probe and other 2xx assertions in this file. */
        constexpr int HTTP_OK = 200;
        /** Coordinates for the first ("drone_02") fixture target in ordering tests. */
        constexpr double FIRST_TARGET_LATITUDE_DEG = 1.0;
        constexpr double FIRST_TARGET_LONGITUDE_DEG = 2.0;
        constexpr double FIRST_TARGET_ALTITUDE_M = 3.0;
        /** Coordinates for the second ("drone_01") fixture target, also asserted on directly. */
        constexpr double SECOND_TARGET_LATITUDE_DEG = 34.05;
        constexpr double SECOND_TARGET_LONGITUDE_DEG = -118.25;
        constexpr double SECOND_TARGET_ALTITUDE_M = 150.0;

        /** @brief Controllable Api_Port test double used to isolate REST transport mapping from core logic. */
        class Fake_Api_Port final : public core::ports::inbound::Api_Port {
        public:
            // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
            // This is a test-only fake with no invariants to protect: tests configure its behavior
            // directly by writing these fields, the same way a fixture's protected state is used.
            edge::telemetry::v1::CuePriorityList cues;
            std::function<edge::Result<core::ports::inbound::Slew_Accepted, core::ports::inbound::Slew_Failure>(
                std::string_view )>
                slew_handler = []( std::string_view target_uid ) {
                    return edge::Result<core::ports::inbound::Slew_Accepted, core::ports::inbound::Slew_Failure>(
                        core::ports::inbound::Slew_Accepted { std::string( target_uid ) } );
                };
            // NOLINTEND(misc-non-private-member-variables-in-classes)

            /** @brief Returns the fake cue snapshot configured by the current test. */
            [[nodiscard]] auto Get_Cues() const -> edge::telemetry::v1::CuePriorityList override { return cues; }

            /** @brief Records the requested UID and returns the test-configured slew result. */
            [[nodiscard]] auto Request_Slew( std::string_view target_uid )
                -> edge::Result<core::ports::inbound::Slew_Accepted, core::ports::inbound::Slew_Failure> override {
                return slew_handler( target_uid );
            }
        };

        /** @brief Polls the test REST readiness endpoint with a hard deadline. */
        [[nodiscard]] auto Wait_For_Ready( httplib::Client& client ) -> bool {
            const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
            while( std::chrono::steady_clock::now() < deadline ) {
                auto response = client.Get( "/api/v1/ready" );
                if( response && response->status == HTTP_OK ) {
                    return true;
                }
            }
            return false;
        }

        /** @brief Fixture owning one REST adapter, fake core port, logger, and loopback client. */
        class Rest_Server_Test : public ::testing::Test {
        protected:
            // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
            // GTest fixtures conventionally expose state via `protected` so that TEST_F-generated
            // subclasses can access it directly; there is no external caller to encapsulate against.
            Fake_Api_Port api;
            edge::platform::logging::Logger logger { "api_gateway_test" };
            Rest_Server server { "127.0.0.1", 0, api, logger };
            httplib::Client client { "127.0.0.1", server.Bound_Port() };
            // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)

            /** @brief Waits for the REST listener to be ready before each test body. */
            void SetUp() override { ASSERT_TRUE( Wait_For_Ready( client ) ) << "Rest_Server never became ready"; }
            /** @brief Stops the REST listener deterministically after each test. */
            void TearDown() override { server.Stop(); }
        };

        /** @test Verifies that ready returns 200. */
        TEST_F( Rest_Server_Test, Ready_Returns_200 ) {
            const auto response = client.Get( "/api/v1/ready" );
            ASSERT_TRUE( response );
            EXPECT_EQ( response->status, 200 );
            EXPECT_EQ( nlohmann::json::parse( response->body ).at( "status" ), "ready" );
        }

        /** @test Verifies that cues on empty port result returns empty array. */
        TEST_F( Rest_Server_Test, Cues_On_Empty_Port_Result_Returns_Empty_Array ) {
            const auto response = client.Get( "/api/v1/cues" );
            ASSERT_TRUE( response );
            EXPECT_EQ( response->status, 200 );
            const auto body = nlohmann::json::parse( response->body );
            ASSERT_TRUE( body.at( "targets" ).is_array() );
            EXPECT_TRUE( body.at( "targets" ).empty() );
        }

        /** @test Verifies that cues are mapped from inbound port in order. */
        TEST_F( Rest_Server_Test, Cues_Are_Mapped_From_Inbound_Port_In_Order ) {
            auto* first = api.cues.add_targets();
            first->set_uid( "drone_02" );
            first->set_latitude_deg( FIRST_TARGET_LATITUDE_DEG );
            first->set_longitude_deg( FIRST_TARGET_LONGITUDE_DEG );
            first->set_altitude_m( FIRST_TARGET_ALTITUDE_M );
            auto* second = api.cues.add_targets();
            second->set_uid( "drone_01" );
            second->set_latitude_deg( SECOND_TARGET_LATITUDE_DEG );
            second->set_longitude_deg( SECOND_TARGET_LONGITUDE_DEG );
            second->set_altitude_m( SECOND_TARGET_ALTITUDE_M );

            const auto response = client.Get( "/api/v1/cues" );
            ASSERT_TRUE( response );
            EXPECT_EQ( response->status, HTTP_OK );
            const auto targets = nlohmann::json::parse( response->body ).at( "targets" );
            ASSERT_EQ( targets.size(), 2U );
            EXPECT_EQ( targets[0].at( "uid" ), "drone_02" );
            EXPECT_EQ( targets[1].at( "uid" ), "drone_01" );
            EXPECT_DOUBLE_EQ( targets[1].at( "latitude_deg" ).get<double>(), SECOND_TARGET_LATITUDE_DEG );
        }

        /** @test Verifies that unknown path returns 404 with json body. */
        TEST_F( Rest_Server_Test, Unknown_Path_Returns_404_With_Json_Body ) {
            const auto response = client.Get( "/api/v1/does-not-exist" );
            ASSERT_TRUE( response );
            EXPECT_EQ( response->status, 404 );
            EXPECT_TRUE( nlohmann::json::parse( response->body ).contains( "error" ) );
        }

        /** @test Verifies that post slew malformed json returns 400. */
        TEST_F( Rest_Server_Test, Post_Slew_Malformed_Json_Returns_400 ) {
            const auto response = client.Post( "/api/v1/slew", "not-json-at-all", "application/json" );
            ASSERT_TRUE( response );
            EXPECT_EQ( response->status, 400 );
            EXPECT_EQ( nlohmann::json::parse( response->body ).at( "error" ), "invalid_request" );
        }

        /** @test Verifies that post slew non object json returns 400. */
        TEST_F( Rest_Server_Test, Post_Slew_Non_Object_Json_Returns_400 ) {
            const auto response = client.Post( "/api/v1/slew", "[1,2,3]", "application/json" );
            ASSERT_TRUE( response );
            EXPECT_EQ( response->status, 400 );
        }

        /** @test Verifies that post slew missing empty or non string target uid returns 400. */
        TEST_F( Rest_Server_Test, Post_Slew_Missing_Empty_Or_Non_String_Target_Uid_Returns_400 ) {
            for( const auto& body : { std::string( "{}" ),
                                      std::string( R"({"target_uid":""})" ),
                                      std::string( R"({"target_uid":42})" ) } ) {
                const auto response = client.Post( "/api/v1/slew", body, "application/json" );
                ASSERT_TRUE( response );
                EXPECT_EQ( response->status, 400 );
            }
        }

        /** @test Verifies that post slew body over limit returns 413. */
        TEST_F( Rest_Server_Test, Post_Slew_Body_Over_Limit_Returns_413 ) {
            const std::string oversized_uid( MAX_REST_BODY_BYTES + 1, 'a' );
            const auto body = nlohmann::json { { "target_uid", oversized_uid } }.dump();
            const auto response = client.Post( "/api/v1/slew", body, "application/json" );
            ASSERT_TRUE( response );
            EXPECT_EQ( response->status, 413 );
        }

        /** @test Verifies that post slew accepted returns 200. */
        TEST_F( Rest_Server_Test, Post_Slew_Accepted_Returns_200 ) {
            const auto response = client.Post( "/api/v1/slew", R"({"target_uid":"drone_01"})", "application/json" );
            ASSERT_TRUE( response );
            EXPECT_EQ( response->status, 200 );
            const auto body = nlohmann::json::parse( response->body );
            EXPECT_EQ( body.at( "status" ), "accepted" );
            EXPECT_EQ( body.at( "target_uid" ), "drone_01" );
        }

        /** @test Verifies that post slew unknown uid returns 404. */
        TEST_F( Rest_Server_Test, Post_Slew_Unknown_Uid_Returns_404 ) {
            api.slew_handler = []( std::string_view target_uid ) {
                return edge::Result<core::ports::inbound::Slew_Accepted, core::ports::inbound::Slew_Failure>(
                    edge::failure,
                    core::ports::inbound::Slew_Failure { core::ports::inbound::Slew_Failure_Reason::UNKNOWN_TARGET,
                                                         std::string( target_uid ) } );
            };
            const auto response = client.Post( "/api/v1/slew", R"({"target_uid":"drone_01"})", "application/json" );
            ASSERT_TRUE( response );
            EXPECT_EQ( response->status, 404 );
        }

        /** @test Verifies that post slew gimbal rejected returns 409. */
        TEST_F( Rest_Server_Test, Post_Slew_Gimbal_Rejected_Returns_409 ) {
            api.slew_handler = []( std::string_view ) {
                return edge::Result<core::ports::inbound::Slew_Accepted, core::ports::inbound::Slew_Failure>(
                    edge::failure,
                    core::ports::inbound::Slew_Failure { core::ports::inbound::Slew_Failure_Reason::GIMBAL_REJECTED,
                                                         "coordinates rejected" } );
            };
            const auto response = client.Post( "/api/v1/slew", R"({"target_uid":"drone_01"})", "application/json" );
            ASSERT_TRUE( response );
            EXPECT_EQ( response->status, 409 );
            EXPECT_FALSE( nlohmann::json::parse( response->body ).contains( "detail" ) );
        }

        /** @test Verifies that post slew gimbal unavailable and queue full return 503. */
        TEST_F( Rest_Server_Test, Post_Slew_Gimbal_Unavailable_And_Queue_Full_Return_503 ) {
            for( const auto reason : { core::ports::inbound::Slew_Failure_Reason::GIMBAL_UNAVAILABLE,
                                       core::ports::inbound::Slew_Failure_Reason::GIMBAL_QUEUE_FULL } ) {
                api.slew_handler = [reason]( std::string_view ) {
                    return edge::Result<core::ports::inbound::Slew_Accepted, core::ports::inbound::Slew_Failure>(
                        edge::failure, core::ports::inbound::Slew_Failure { reason, "unavailable" } );
                };
                const auto response = client.Post( "/api/v1/slew", R"({"target_uid":"drone_01"})", "application/json" );
                ASSERT_TRUE( response );
                EXPECT_EQ( response->status, 503 );
            }
        }

        /** @test Verifies that unexpected core exceptions are reported as server failures, not client errors. */
        TEST_F( Rest_Server_Test, Post_Slew_Unexpected_Core_Exception_Returns_500 ) {
            api.slew_handler = []( std::string_view )
                -> edge::Result<core::ports::inbound::Slew_Accepted, core::ports::inbound::Slew_Failure> {
                throw std::runtime_error( "synthetic core failure" );
            };

            const auto response = client.Post( "/api/v1/slew", R"({"target_uid":"drone_01"})", "application/json" );
            ASSERT_TRUE( response );
            EXPECT_EQ( response->status, 500 );
            EXPECT_EQ( nlohmann::json::parse( response->body ).at( "error" ), "internal_error" );
        }

        /** @test Verifies that post slew gimbal timeout returns 504 with ambiguous outcome. */
        TEST_F( Rest_Server_Test, Post_Slew_Gimbal_Timeout_Returns_504_With_Ambiguous_Outcome ) {
            api.slew_handler = []( std::string_view ) {
                return edge::Result<core::ports::inbound::Slew_Accepted, core::ports::inbound::Slew_Failure>(
                    edge::failure,
                    core::ports::inbound::Slew_Failure { core::ports::inbound::Slew_Failure_Reason::GIMBAL_TIMEOUT,
                                                         "no reply" } );
            };
            const auto response = client.Post( "/api/v1/slew", R"({"target_uid":"drone_01"})", "application/json" );
            ASSERT_TRUE( response );
            EXPECT_EQ( response->status, 504 );
            EXPECT_EQ( nlohmann::json::parse( response->body ).at( "outcome" ), "unknown" );
        }

        /** @test Verifies downstream hardware diagnostics are not reflected verbatim to external clients. */
        TEST_F( Rest_Server_Test, Hardware_Error_Returns_503_Without_Reflecting_Detail ) {
            api.slew_handler = []( std::string_view ) {
                return edge::Result<core::ports::inbound::Slew_Accepted, core::ports::inbound::Slew_Failure>(
                    edge::failure,
                    core::ports::inbound::Slew_Failure {
                        core::ports::inbound::Slew_Failure_Reason::GIMBAL_HARDWARE_ERROR,
                        "sensitive downstream hardware detail",
                    } );
            };

            const auto response = client.Post( "/api/v1/slew", R"({"target_uid":"drone_01"})", "application/json" );

            ASSERT_TRUE( response );
            EXPECT_EQ( response->status, 503 );
            const auto body = nlohmann::json::parse( response->body );
            EXPECT_EQ( body.at( "error" ), "gimbal_hardware_error" );
            EXPECT_FALSE( body.contains( "detail" ) );
        }

    }  // namespace
}  // namespace edge::api_gateway::adapters::inbound::rest
