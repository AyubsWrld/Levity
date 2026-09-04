/**
 * @file config_test.cpp
 * @brief Test implementation for config test behavior and regression coverage.
 *
 * @details Source path: `services/gimbal_controller/test/runtime/config_test.cpp`.
 */

#include <edge/gimbal_controller/runtime/config.h>
#include <edge/test/scoped_environment.h>
#include <gtest/gtest.h>

namespace edge::gimbal_controller::runtime {
    namespace {

        /** @test Verifies that defaults are loaded when environment is unset. */
        TEST( Gimbal_Controller_Config_Test, Defaults_Are_Loaded_When_Environment_Is_Unset ) {
            const edge::test::Scoped_Environment command { "EDGE_GIMBAL_COMMAND_ENDPOINT" };
            const edge::test::Scoped_Environment host { "EDGE_STANAG_UDP_HOST" };
            const edge::test::Scoped_Environment port { "EDGE_STANAG_UDP_PORT" };
            command.Unset();
            host.Unset();
            port.Unset();

            const auto config = Load_Config();

            EXPECT_FALSE( config.command_endpoint.empty() );
            EXPECT_EQ( config.stanag_udp.host, "127.0.0.1" );
            EXPECT_EQ( config.stanag_udp.port, 14550U );
        }

        /** @test Verifies that environment overrides are applied at runtime boundary. */
        TEST( Gimbal_Controller_Config_Test, Environment_Overrides_Are_Applied_At_Runtime_Boundary ) {
            const edge::test::Scoped_Environment command { "EDGE_GIMBAL_COMMAND_ENDPOINT" };
            const edge::test::Scoped_Environment host { "EDGE_STANAG_UDP_HOST" };
            const edge::test::Scoped_Environment port { "EDGE_STANAG_UDP_PORT" };
            command.Set( "ipc:///tmp/test-gimbal.ipc" );
            host.Set( "127.0.0.2" );
            port.Set( "14999" );

            const auto config = Load_Config();

            EXPECT_EQ( config.command_endpoint, "ipc:///tmp/test-gimbal.ipc" );
            EXPECT_EQ( config.stanag_udp.host, "127.0.0.2" );
            EXPECT_EQ( config.stanag_udp.port, 14999U );
        }

        /** @test Verifies that invalid udp port falls back to default. */
        TEST( Gimbal_Controller_Config_Test, Invalid_Udp_Port_Falls_Back_To_Default ) {
            const edge::test::Scoped_Environment port { "EDGE_STANAG_UDP_PORT" };

            for( const char* invalid_port : { "65536", "99999", "0", "14550x", "not-a-port", "-1" } ) {
                port.Set( invalid_port );
                EXPECT_EQ( Load_Config().stanag_udp.port, 14550U );
            }
        }

    }  // namespace
}  // namespace edge::gimbal_controller::runtime
