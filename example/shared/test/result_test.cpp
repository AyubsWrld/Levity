/**
 * @file result_test.cpp
 * @brief Unit coverage for the C++20 edge::Result success/error utility.
 */

#include <edge/result.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace edge {
    namespace {

        /** @test Verifies value results expose success state and stored values. */
        TEST( Result_Test, Value_Result_Stores_Success ) {
            constexpr int SUCCESS_VALUE = 42;
            Result<int, std::string> result { SUCCESS_VALUE };

            ASSERT_TRUE( result.has_value() );
            EXPECT_TRUE( static_cast<bool>( result ) );
            EXPECT_EQ( result.value(), SUCCESS_VALUE );
            EXPECT_EQ( *result, SUCCESS_VALUE );
        }

        /** @test Verifies failure wrappers construct the error alternative. */
        TEST( Result_Test, Make_Failure_Stores_Error ) {
            Result<int, std::string> result = Make_Failure( std::string { "failure" } );

            ASSERT_FALSE( result.has_value() );
            EXPECT_FALSE( static_cast<bool>( result ) );
            EXPECT_EQ( result.error(), "failure" );
        }

        /** @test Verifies move-only success values are supported without requiring copies. */
        TEST( Result_Test, Supports_Move_Only_Success_Value ) {
            constexpr int MOVE_ONLY_VALUE = 7;
            Result<std::unique_ptr<int>, std::string> result { std::make_unique<int>( MOVE_ONLY_VALUE ) };

            ASSERT_TRUE( result.has_value() );
            ASSERT_NE( result->get(), nullptr );
            EXPECT_EQ( **result, MOVE_ONLY_VALUE );
        }

        /** @test Verifies the void specialization distinguishes success from failure. */
        TEST( Result_Test, Void_Result_Represents_Status ) {
            const Result<void, std::string> success;
            const Result<void, std::string> failure_result = Make_Failure( std::string { "failure" } );

            EXPECT_TRUE( success.has_value() );
            ASSERT_FALSE( failure_result.has_value() );
            EXPECT_EQ( failure_result.error(), "failure" );
        }

        /** @test Verifies the explicit failure tag supports direct typed error construction. */
        TEST( Result_Test, Failure_Tag_Constructs_Error_State ) {
            Result<int, std::string> result { failure, "explicit failure" };

            ASSERT_FALSE( result.has_value() );
            EXPECT_EQ( result.error(), "explicit failure" );
        }

    }  // namespace
}  // namespace edge
