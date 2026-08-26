/**
 * @file endpoint_test.cpp
 * @brief Test implementation for endpoint test behavior and regression coverage.
 *
 * @details Source path: `shared/libs/ipc/test/endpoint_test.cpp`.
 */

#include <edge/ipc/context.h>
#include <edge/ipc/endpoints.h>
#include <edge/ipc/error.h>
#include <edge/ipc/publisher.h>
#include <edge/ipc/replier.h>
#include <edge/test/temp_dir.h>
#include <gtest/gtest.h>
#include <sys/stat.h>

#include <filesystem>
#include <string>

namespace edge::ipc {
    namespace {

        /** @test Verifies that matches tier1 contract. */
        TEST( Default_Endpoint_Test, Matches_Tier1_Contract ) {
            EXPECT_EQ( DEFAULT_TELEMETRY_ENDPOINT, "ipc:///tmp/edge-ipc/telemetry.ipc" );
            EXPECT_EQ( DEFAULT_GIMBAL_COMMAND_ENDPOINT, "ipc:///tmp/edge-ipc/gimbal-cmd.ipc" );
        }

        /** @test Verifies that creates missing parent directory. */
        TEST( Publisher_Bind_Test, Creates_Missing_Parent_Directory ) {
            const edge::test::Temp_Dir temp_dir;
            const auto parent = std::filesystem::path( temp_dir.Path() ) / "nested" / "ipc";
            const std::string endpoint = "ipc://" + ( parent / "telemetry.ipc" ).string();

            ASSERT_FALSE( std::filesystem::exists( parent ) );

            Context context;
            const auto publisher = Publisher::Bind( context, endpoint );

            ASSERT_TRUE( publisher.has_value() ) << publisher.error().detail;
            ASSERT_TRUE( std::filesystem::exists( parent ) );

            struct stat info {};
            ASSERT_EQ( ::stat( parent.c_str(), &info ), 0 );
            EXPECT_EQ( info.st_mode & 0777U, 0700U );

            struct stat intermediate_info {};
            ASSERT_EQ( ::stat( ( std::filesystem::path( temp_dir.Path() ) / "nested" ).c_str(), &intermediate_info ),
                       0 );
            EXPECT_EQ( intermediate_info.st_mode & 0777U, 0700U );
        }

        /** @test Verifies that does not change permissions of existing parent. */
        TEST( Publisher_Bind_Test, Does_Not_Change_Permissions_Of_Existing_Parent ) {
            const edge::test::Temp_Dir temp_dir;
            const auto parent = std::filesystem::path( temp_dir.Path() ) / "existing";
            ASSERT_TRUE( std::filesystem::create_directory( parent ) );
            std::filesystem::permissions( parent,
                                          std::filesystem::perms::owner_all | std::filesystem::perms::group_read,
                                          std::filesystem::perm_options::replace );

            struct stat before {};
            ASSERT_EQ( ::stat( parent.c_str(), &before ), 0 );

            Context context;
            const auto publisher = Publisher::Bind( context, "ipc://" + ( parent / "telemetry.ipc" ).string() );

            ASSERT_TRUE( publisher.has_value() ) << publisher.error().detail;
            struct stat after {};
            ASSERT_EQ( ::stat( parent.c_str(), &after ), 0 );
            EXPECT_EQ( after.st_mode & 0777U, before.st_mode & 0777U );
        }

        /** @test Verifies that creates missing parent directory. */
        TEST( Replier_Bind_Test, Creates_Missing_Parent_Directory ) {
            const edge::test::Temp_Dir temp_dir;
            const auto parent = std::filesystem::path( temp_dir.Path() ) / "nested";
            const std::string endpoint = "ipc://" + ( parent / "command.ipc" ).string();

            Context context;
            const auto replier = Replier::Bind( context, endpoint );

            ASSERT_TRUE( replier.has_value() ) << replier.error().detail;
            EXPECT_TRUE( std::filesystem::exists( parent ) );
        }

        /** @test Verifies that rejects non ipc endpoint before touching zeromq. */
        TEST( Bind_Test, Rejects_Non_Ipc_Endpoint_Before_Touching_ZeroMq ) {
            Context context;

            const auto result = Publisher::Bind( context, "tcp://127.0.0.1:5555" );

            ASSERT_FALSE( result.has_value() );
            EXPECT_EQ( result.error().code, Error_Code::INVALID_ENDPOINT );
        }

    }  // namespace
}  // namespace edge::ipc
