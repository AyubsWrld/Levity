/**
 * @file max_message_size_test.cpp
 * @brief Test implementation for max message size test behavior and regression coverage.
 *
 * @details Source path: `shared/libs/ipc/test/max_message_size_test.cpp`.
 */

// Transport-level enforcement of MAX_SERIALIZED_MESSAGE_BYTES.
//
// The application-level size checks in publisher/subscriber/requester/replier run only *after*
// `recv()` returns, by which point libzmq has already read the peer's declared frame length and
// allocated a buffer for it. `Configure_New_Socket()` therefore also sets `ZMQ_MAXMSGSIZE`, so
// libzmq itself refuses an oversized frame at the transport layer.
//
// These tests deliberately use a RAW cppzmq socket as the sending peer: every sender in this
// library rejects an oversized payload before sending, so a non-conforming peer is the only way to
// exercise the receive side.
#include "edge/ipc/context.h"
#include "edge/ipc/limits.h"
#include "edge/ipc/subscriber.h"

#include <edge/test/temp_dir.h>
#include <gtest/gtest.h>
#include <zmq.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace edge::ipc {
    namespace {

        constexpr auto TEST_DEADLINE = std::chrono::seconds( 5 );
        constexpr auto RECEIVE_SLICE = std::chrono::milliseconds( 50 );

        // Sends `payload` on a raw PUB socket until the subscriber reports a message or the deadline
        // expires. Returns the first message actually delivered, if any.
        /** @brief Repeatedly publishes from a raw peer until the wrapper observes a message or the deadline expires. */
        [[nodiscard]] auto Publish_Until_Received( zmq::socket_t& raw_publisher,
                                                   Subscriber& subscriber,
                                                   const std::string& payload ) -> std::optional<Message> {
            const auto deadline = std::chrono::steady_clock::now() + TEST_DEADLINE;
            while( std::chrono::steady_clock::now() < deadline ) {
                raw_publisher.send( zmq::buffer( std::string( "cues" ) ), zmq::send_flags::sndmore );
                raw_publisher.send( zmq::buffer( payload ), zmq::send_flags::none );

                auto receive_result = subscriber.Receive( RECEIVE_SLICE );
                if( receive_result.has_value() && receive_result->has_value() ) {
                    return std::move( **receive_result );
                }
            }
            return std::nullopt;
        }

        /** @brief Fixture combining a raw libzmq peer with the public wrapper to test receive-side size enforcement. */
        class Max_Message_Size_Test : public ::testing::Test {
        protected:
            Max_Message_Size_Test()
                : m_endpoint( "ipc://" + m_temp_dir.Path() + "/maxmsgsize.ipc" ),
                  m_raw_context( 1 ),
                  m_raw_publisher( m_raw_context, zmq::socket_type::pub ) {
                m_raw_publisher.set( zmq::sockopt::linger, 0 );
                // The non-conforming peer keeps libzmq's default (unlimited) outbound bound, so it
                // can genuinely put an oversized frame on the wire.
                m_raw_publisher.bind( m_endpoint );
            }

            // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
            // GTest fixtures conventionally expose state via `protected` so that TEST_F-generated
            // subclasses can access it directly; there is no external caller to encapsulate against.
            edge::test::Temp_Dir m_temp_dir;
            std::string m_endpoint;
            zmq::context_t m_raw_context;
            zmq::socket_t m_raw_publisher;
            Context m_context;
            // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
        };

        // Control case: the harness itself works, so a negative result below is meaningful.
        /** @test Verifies that conforming payload from raw peer is delivered. */
        TEST_F( Max_Message_Size_Test, Conforming_Payload_From_Raw_Peer_Is_Delivered ) {
            auto subscriber_result = Subscriber::Connect( m_context, m_endpoint, "cues" );
            ASSERT_TRUE( subscriber_result.has_value() );

            const std::string at_limit( MAX_SERIALIZED_MESSAGE_BYTES, 'a' );
            const auto received = Publish_Until_Received( m_raw_publisher, *subscriber_result, at_limit );

            ASSERT_TRUE( received.has_value() );
            // The optional is checked immediately above; suppress the conservative unchecked-
            // optional-access diagnostic around the GTest control-flow macro expansion.
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            EXPECT_EQ( received->payload.size(), MAX_SERIALIZED_MESSAGE_BYTES );
        }

        // Regression: without ZMQ_MAXMSGSIZE, libzmq would allocate a buffer for whatever size the
        // peer declared before this library could reject it - an unbounded, peer-controlled
        // allocation. The oversized frame must never be delivered to the application at all.
        /** @test Verifies that oversized payload from raw peer is never delivered. */
        TEST_F( Max_Message_Size_Test, Oversized_Payload_From_Raw_Peer_Is_Never_Delivered ) {
            auto subscriber_result = Subscriber::Connect( m_context, m_endpoint, "cues" );
            ASSERT_TRUE( subscriber_result.has_value() );

            const std::string oversized( MAX_SERIALIZED_MESSAGE_BYTES + 1, 'a' );
            const auto received = Publish_Until_Received( m_raw_publisher, *subscriber_result, oversized );

            EXPECT_FALSE( received.has_value() )
                << "an oversized frame reached the application; ZMQ_MAXMSGSIZE is not in effect";
        }

    }  // namespace
}  // namespace edge::ipc
