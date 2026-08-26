/**
 * @file zmq_telemetry_subscriber_test.cpp
 * @brief Test implementation for zmq telemetry subscriber test behavior and regression coverage.
 *
 * @details Source path: `services/api_gateway/test/adapters/inbound/telemetry/zmq_telemetry_subscriber_test.cpp`.
 */

#include "telemetry.pb.h"

#include <edge/api_gateway/adapters/inbound/telemetry/zmq_telemetry_subscriber.h>
#include <edge/api_gateway/core/ports/inbound/telemetry_port.h>
#include <edge/ipc/context.h>
#include <edge/ipc/publisher.h>
#include <edge/platform/logging/logger.h>
#include <edge/test/temp_dir.h>
#include <gtest/gtest.h>

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>

namespace edge::api_gateway::adapters::inbound::telemetry {
    namespace {

        constexpr auto TEST_DEADLINE = std::chrono::seconds( 5 );
        constexpr auto PUBLISH_INTERVAL = std::chrono::milliseconds( 20 );
        /** Sample coordinates used by the deterministic single-target test fixture. */
        constexpr double SAMPLE_LATITUDE_DEG = 34.05;
        constexpr double SAMPLE_LONGITUDE_DEG = -118.25;
        constexpr double SAMPLE_ALTITUDE_M = 150.0;
        /** Number of malformed-frame publications sent while proving the port is never invoked. */
        constexpr int MALFORMED_PUBLISH_ATTEMPTS = 10;

        /** @brief Thread-safe inbound-port fake recording the latest telemetry snapshot delivered by the adapter. */
        class Fake_Telemetry_Port final : public core::ports::inbound::Telemetry_Port {
        public:
            /** @brief Records the latest delivered cue list under a mutex. */
            void Update_Cues( edge::telemetry::v1::CuePriorityList cues ) override {
                const std::scoped_lock lock( m_mutex );
                m_last = std::move( cues );
            }

            /** @brief Returns a thread-safe copy of the most recently recorded telemetry update. */
            [[nodiscard]] auto Snapshot() const -> std::optional<edge::telemetry::v1::CuePriorityList> {
                const std::scoped_lock lock( m_mutex );
                return m_last;
            }

        private:
            mutable std::mutex m_mutex;
            std::optional<edge::telemetry::v1::CuePriorityList> m_last;
        };

        /** @brief Builds a minimal telemetry snapshot with one deterministic UID. */
        [[nodiscard]] auto Make_List( std::string uid ) -> edge::telemetry::v1::CuePriorityList {
            edge::telemetry::v1::CuePriorityList list;
            auto* target = list.add_targets();
            target->set_uid( std::move( uid ) );
            target->set_latitude_deg( SAMPLE_LATITUDE_DEG );
            target->set_longitude_deg( SAMPLE_LONGITUDE_DEG );
            target->set_altitude_m( SAMPLE_ALTITUDE_M );
            return list;
        }

        /** @test Verifies that valid telemetry is forwarded through inbound port. */
        TEST( Zmq_Telemetry_Subscriber_Test, Valid_Telemetry_Is_Forwarded_Through_Inbound_Port ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/telemetry.ipc";

            edge::ipc::Context publisher_context;
            auto publisher_result = edge::ipc::Publisher::Bind( publisher_context, endpoint );
            ASSERT_TRUE( publisher_result.has_value() );

            edge::ipc::Context subscriber_context;
            Fake_Telemetry_Port telemetry;
            edge::platform::logging::Logger logger { "api_gateway_test" };
            const Zmq_Telemetry_Subscriber subscriber( subscriber_context, endpoint, telemetry, logger );

            std::string payload;
            ASSERT_TRUE( Make_List( "drone_01" ).SerializeToString( &payload ) );

            const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
            while( std::chrono::steady_clock::now() < deadline ) {
                ASSERT_TRUE( publisher_result->Publish( "cues", payload ).has_value() );

                const auto snapshot = telemetry.Snapshot();
                if( snapshot.has_value() && snapshot->targets_size() == 1 &&
                    snapshot->targets( 0 ).uid() == "drone_01" ) {
                    return;
                }
                std::this_thread::sleep_for( PUBLISH_INTERVAL );
            }

            FAIL() << "telemetry was not forwarded through Telemetry_Port before deadline";
        }

        /** @test Verifies that malformed telemetry does not call inbound port. */
        TEST( Zmq_Telemetry_Subscriber_Test, Malformed_Telemetry_Does_Not_Call_Inbound_Port ) {
            const edge::test::Temp_Dir temp_dir;
            const std::string endpoint = "ipc://" + temp_dir.Path() + "/telemetry.ipc";

            edge::ipc::Context publisher_context;
            auto publisher_result = edge::ipc::Publisher::Bind( publisher_context, endpoint );
            ASSERT_TRUE( publisher_result.has_value() );

            edge::ipc::Context subscriber_context;
            Fake_Telemetry_Port telemetry;
            edge::platform::logging::Logger logger { "api_gateway_test" };
            const Zmq_Telemetry_Subscriber subscriber( subscriber_context, endpoint, telemetry, logger );

            for( int i = 0; i < MALFORMED_PUBLISH_ATTEMPTS; ++i ) {
                ASSERT_TRUE( publisher_result->Publish( "cues", std::string( "\xFF\xFE\xFD", 3 ) ).has_value() );
                std::this_thread::sleep_for( PUBLISH_INTERVAL );
            }

            EXPECT_FALSE( telemetry.Snapshot().has_value() );
        }

        /** @test Verifies that invalid endpoint is startup failure. */
        TEST( Zmq_Telemetry_Subscriber_Test, Invalid_Endpoint_Is_Startup_Failure ) {
            edge::ipc::Context context;
            Fake_Telemetry_Port telemetry;
            edge::platform::logging::Logger logger { "api_gateway_test" };

            EXPECT_THROW( ( Zmq_Telemetry_Subscriber { context, "not-an-ipc-endpoint", telemetry, logger } ),
                          std::runtime_error );
        }

    }  // namespace
}  // namespace edge::api_gateway::adapters::inbound::telemetry
