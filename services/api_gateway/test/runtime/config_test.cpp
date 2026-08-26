/**
 * @file config_test.cpp
 * @brief Test implementation for config test behavior and regression coverage.
 *
 * @details Source path: `services/api_gateway/test/runtime/config_test.cpp`.
 */

#include <edge/api_gateway/runtime/config.h>
#include <edge/test/scoped_environment.h>
#include <gtest/gtest.h>

#include <stdexcept>

namespace edge::api_gateway::runtime {
    namespace {

        /** @test Verifies that defaults are loaded when environment is unset. */
        TEST( Api_Gateway_Config_Test, Defaults_Are_Loaded_When_Environment_Is_Unset ) {
            const edge::test::Scoped_Environment telemetry { "EDGE_TELEMETRY_ENDPOINT" };
            const edge::test::Scoped_Environment command { "EDGE_GIMBAL_COMMAND_ENDPOINT" };
            const edge::test::Scoped_Environment host { "EDGE_HTTP_HOST" };
            const edge::test::Scoped_Environment port { "EDGE_HTTP_PORT" };
            telemetry.Unset();
            command.Unset();
            host.Unset();
            port.Unset();

            const auto config = Load_Config();

            EXPECT_FALSE( config.telemetry_endpoint.empty() );
            EXPECT_FALSE( config.gimbal_command_endpoint.empty() );
            EXPECT_EQ( config.http.host, "127.0.0.1" );
            EXPECT_EQ( config.http.port, 8080U );
        }

        /** @test Verifies that environment overrides are applied once at runtime boundary. */
        TEST( Api_Gateway_Config_Test, Environment_Overrides_Are_Applied_Once_At_Runtime_Boundary ) {
            const edge::test::Scoped_Environment telemetry { "EDGE_TELEMETRY_ENDPOINT" };
            const edge::test::Scoped_Environment command { "EDGE_GIMBAL_COMMAND_ENDPOINT" };
            const edge::test::Scoped_Environment host { "EDGE_HTTP_HOST" };
            const edge::test::Scoped_Environment port { "EDGE_HTTP_PORT" };
            telemetry.Set( "ipc:///tmp/test-telemetry.ipc" );
            command.Set( "ipc:///tmp/test-command.ipc" );
            host.Set( "0.0.0.0" );
            port.Set( "18080" );

            const auto config = Load_Config();

            EXPECT_EQ( config.telemetry_endpoint, "ipc:///tmp/test-telemetry.ipc" );
            EXPECT_EQ( config.gimbal_command_endpoint, "ipc:///tmp/test-command.ipc" );
            EXPECT_EQ( config.http.host, "0.0.0.0" );
            EXPECT_EQ( config.http.port, 18080U );
        }

        /** @test Verifies that invalid http port is a startup configuration error. */
        TEST( Api_Gateway_Config_Test, Invalid_Http_Port_Is_A_Startup_Configuration_Error ) {
            const edge::test::Scoped_Environment port { "EDGE_HTTP_PORT" };
            port.Set( "99999" );
            EXPECT_THROW( (void)Load_Config(), std::runtime_error );

            port.Set( "not-a-port" );
            EXPECT_THROW( (void)Load_Config(), std::runtime_error );
        }

    }  // namespace
}  // namespace edge::api_gateway::runtime
