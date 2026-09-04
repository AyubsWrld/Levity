/**
 * @file zmq_telemetry_subscriber.cpp
 * @brief Implementation of zmq telemetry subscriber.
 *
 * @details Source path: `services/api_gateway/src/adapters/inbound/telemetry/zmq_telemetry_subscriber.cpp`.
 */

#include "telemetry.pb.h"

#include <edge/api_gateway/adapters/inbound/telemetry/zmq_telemetry_subscriber.h>
#include <edge/ipc/subscriber.h>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace edge::api_gateway::adapters::inbound::telemetry {

    namespace {
        constexpr auto RECEIVE_TIMEOUT = std::chrono::milliseconds( 100 );
        constexpr std::string_view TELEMETRY_TOPIC = "cues";
    }  // namespace

    /** @copydoc Zmq_Telemetry_Subscriber::Zmq_Telemetry_Subscriber */
    Zmq_Telemetry_Subscriber::Zmq_Telemetry_Subscriber( edge::ipc::Context& context,
                                                        std::string endpoint,
                                                        core::ports::inbound::Telemetry_Port& telemetry,
                                                        edge::platform::logging::Logger& logger ) {
        auto ready_promise = std::make_shared<std::promise<Startup_Result>>();
        auto ready_future = ready_promise->get_future();

        m_worker = std::jthread( [&context, endpoint = std::move( endpoint ), &telemetry, &logger, ready_promise](
                                     const std::stop_token& stop_token ) {
            Run( stop_token, context, endpoint, telemetry, logger, *ready_promise );
        } );

        auto startup_result = ready_future.get();
        if( !startup_result.has_value() ) {
            m_worker.request_stop();
            m_worker.join();
            throw std::runtime_error( "Zmq_Telemetry_Subscriber: " + startup_result.error() );
        }
    }

    /** @copydoc Zmq_Telemetry_Subscriber::~Zmq_Telemetry_Subscriber */
    Zmq_Telemetry_Subscriber::~Zmq_Telemetry_Subscriber() { Stop(); }

    /** @copydoc Zmq_Telemetry_Subscriber::Stop */
    void Zmq_Telemetry_Subscriber::Stop() noexcept {
        if( m_worker.joinable() ) {
            m_worker.request_stop();
            m_worker.join();
        }
    }

    /** @copydoc Zmq_Telemetry_Subscriber::Run */
    void Zmq_Telemetry_Subscriber::Run( const std::stop_token& stop_token,
                                        edge::ipc::Context& context,
                                        const std::string& endpoint,
                                        core::ports::inbound::Telemetry_Port& telemetry,
                                        edge::platform::logging::Logger& logger,
                                        std::promise<Startup_Result>& ready_promise ) {
        auto subscriber_result = [&]() -> edge::Result<edge::ipc::Subscriber, std::string> {
            try {
                auto result = edge::ipc::Subscriber::Connect( context, endpoint, TELEMETRY_TOPIC );
                if( !result.has_value() ) {
                    return edge::Make_Failure( result.error().detail );
                }
                return std::move( *result );
            } catch( const std::exception& error ) {
                return edge::Make_Failure( std::string( "subscriber startup exception: " ) + error.what() );
            } catch( ... ) {
                return edge::Make_Failure( std::string( "subscriber startup failed with an unknown exception" ) );
            }
        }();
        if( !subscriber_result.has_value() ) {
            ready_promise.set_value( edge::Make_Failure( subscriber_result.error() ) );
            return;
        }
        ready_promise.set_value( Startup_Result {} );

        auto& subscriber = *subscriber_result;
        while( !stop_token.stop_requested() ) {
            auto receive_result = subscriber.Receive( RECEIVE_TIMEOUT );
            if( !receive_result.has_value() ) {
                logger.Warn( "telemetry_receive_failed", { { "detail", receive_result.error().detail } } );
                continue;
            }
            if( !receive_result->has_value() ) {
                continue;
            }

            edge::telemetry::v1::CuePriorityList list;
            // The optional is checked immediately above; suppress the conservative unchecked-
            // optional-access diagnostic.
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            if( !list.ParseFromString( ( *receive_result )->payload ) ) {
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                logger.Warn( "telemetry_parse_failed", { { "topic", ( *receive_result )->topic } } );
                continue;
            }

            telemetry.Update_Cues( std::move( list ) );
        }
    }

}  // namespace edge::api_gateway::adapters::inbound::telemetry
