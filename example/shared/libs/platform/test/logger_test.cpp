/**
 * @file logger_test.cpp
 * @brief Test implementation for logger test behavior and regression coverage.
 *
 * @details Source path: `shared/libs/platform/test/logger_test.cpp`.
 */

#include <edge/platform/logging/logger.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

namespace edge::platform::logging {
    namespace {

        // Captures everything `Logger` writes to stderr for the lifetime of the object, then
        // restores the original stream buffer so later tests still report normally.
        /** @brief RAII redirector capturing stderr so structured Logger output can be asserted byte-for-byte. */
        class Stderr_Capture final {
        public:
            Stderr_Capture() : m_previous( std::cerr.rdbuf( m_buffer.rdbuf() ) ) {}

            Stderr_Capture( const Stderr_Capture& ) = delete;
            auto operator=( const Stderr_Capture& ) -> Stderr_Capture& = delete;
            // Swaps a process-global stream buffer pointer on construction/destruction; moving an
            // already-active capture would leave two objects racing to restore the same buffer.
            Stderr_Capture( Stderr_Capture&& ) = delete;
            auto operator=( Stderr_Capture&& ) -> Stderr_Capture& = delete;

            ~Stderr_Capture() { std::cerr.rdbuf( m_previous ); }

            /** @brief Returns all stderr text captured since this helper was constructed. */
            [[nodiscard]] auto Text() const -> std::string { return m_buffer.str(); }

        private:
            std::ostringstream m_buffer;
            std::streambuf* m_previous;
        };

        /** @test Verifies that emits prefix and fields. */
        TEST( Logger_Test, Emits_Prefix_And_Fields ) {
            const Stderr_Capture capture;
            Logger { "api_gateway" }.Info( "slew_accepted", { { "target_uid", "TGT-001" } } );

            const auto text = capture.Text();
            EXPECT_NE( text.find( "service=api_gateway" ), std::string::npos );
            EXPECT_NE( text.find( "severity=info" ), std::string::npos );
            EXPECT_NE( text.find( "event=slew_accepted" ), std::string::npos );
            EXPECT_NE( text.find( "target_uid=TGT-001" ), std::string::npos );
        }

        // Regression: a client-supplied uid containing a newline previously ended the log line and
        // let the remainder be read as a separate, forged log record.
        /** @test Verifies that newline in value cannot forge a second log line. */
        TEST( Logger_Test, Newline_In_Value_Cannot_Forge_A_Second_Log_Line ) {
            const Stderr_Capture capture;
            Logger { "api_gateway" }.Info( "slew_rejected",
                                           { { "target_uid", "evil\nts=1970-01-01T00:00:00Z severity=info" } } );

            const auto text = capture.Text();
            // Exactly one line is emitted: the trailing newline written by Logger itself.
            EXPECT_EQ( std::count( text.begin(), text.end(), '\n' ), 1 );
            EXPECT_EQ( text.find( "evil\nts=" ), std::string::npos );
            EXPECT_NE( text.find( "evil\\x0Ats=" ), std::string::npos );
        }

        /** @test Verifies that carriage return and escape sequences are neutralised. */
        TEST( Logger_Test, Carriage_Return_And_Escape_Sequences_Are_Neutralised ) {
            const Stderr_Capture capture;
            Logger { "api_gateway" }.Warn( "telemetry_rejected", { { "target_uid", "a\rb\x1b[31mc\x7f" } } );

            const auto text = capture.Text();
            EXPECT_EQ( text.find( '\r' ), std::string::npos );
            EXPECT_EQ( text.find( '\x1b' ), std::string::npos );
            EXPECT_EQ( text.find( '\x7f' ), std::string::npos );
            EXPECT_NE( text.find( "a\\x0Db\\x1B[31mc\\x7F" ), std::string::npos );
        }

        /** @test Verifies that over long value is truncated. */
        TEST( Logger_Test, Over_Long_Value_Is_Truncated ) {
            const std::string long_uid( 4096, 'A' );

            const Stderr_Capture capture;
            Logger { "api_gateway" }.Info( "slew_rejected", { { "target_uid", long_uid } } );

            const auto text = capture.Text();
            EXPECT_NE( text.find( "...<truncated>" ), std::string::npos );
            // The emitted line stays bounded rather than echoing the whole 4 KiB value back.
            EXPECT_LT( text.size(), long_uid.size() );
        }

        /** @test Verifies that printable values are left unchanged. */
        TEST( Logger_Test, Printable_Values_Are_Left_Unchanged ) {
            const Stderr_Capture capture;
            Logger { "api_gateway" }.Info( "slew_accepted", { { "path", "/api/v1/slew" }, { "result", "ok" } } );

            const auto text = capture.Text();
            EXPECT_NE( text.find( "path=/api/v1/slew" ), std::string::npos );
            EXPECT_NE( text.find( "result=ok" ), std::string::npos );
            EXPECT_EQ( text.find( "\\x" ), std::string::npos );
        }

    }  // namespace
}  // namespace edge::platform::logging
