/**
 * @file ipc_latency_bench.cpp
 * @brief Standalone IPC latency benchmark used for development regression measurements.
 *
 * @details Source path: `shared/libs/ipc/bench/ipc_latency_bench.cpp`.
 */

// Development-container IPC latency measurement tool.
//
// Prints p50/p95/p99 round-trip latency (REQ/REP) and one-way latency (PUB/SUB) in microseconds.
// This is a measurement tool, not a correctness test: it is intentionally NOT registered with
// CTest. See the disclaimer printed at startup and the `zeromq-ipc` skill's Performance section.
//
// Usage: edge_ipc_bench [iterations] [payload_size_bytes]
#include <edge/ipc/context.h>
#include <edge/ipc/publisher.h>
#include <edge/ipc/replier.h>
#include <edge/ipc/requester.h>
#include <edge/ipc/subscriber.h>
#include <edge/test/temp_dir.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

namespace {

    using edge::ipc::Context;
    using edge::ipc::Message;
    using edge::ipc::Publisher;
    using edge::ipc::Replier;
    using edge::ipc::Requester;
    using edge::ipc::Subscriber;

    /** Hard deadline for establishing each benchmark transport topology. */
    constexpr auto CONNECT_DEADLINE = std::chrono::seconds( 5 );
    /** Short timeout used while probing initial connectivity/readiness. */
    constexpr auto PER_ATTEMPT_TIMEOUT = std::chrono::milliseconds( 200 );
    /** Per-sample timeout used after benchmark connectivity is established. */
    constexpr auto STEADY_STATE_TIMEOUT = std::chrono::milliseconds( 1000 );
    /** Poll interval used by the REQ/REP benchmark's dedicated Replier thread. */
    constexpr auto REPLIER_POLL_TIMEOUT = std::chrono::milliseconds( 100 );
    /** Default steady-state sample count when not overridden on the command line. */
    constexpr std::size_t DEFAULT_ITERATIONS = 1000;
    /** Default application payload size in bytes when not overridden on the command line. */
    constexpr std::size_t DEFAULT_PAYLOAD_SIZE = 256;
    /** Percentile fractions reported by Compute_Percentiles(). */
    constexpr double P50_FRACTION = 0.50;
    constexpr double P95_FRACTION = 0.95;
    constexpr double P99_FRACTION = 0.99;

    /** @brief Command-line-controlled benchmark sample count and payload size. */
    struct Bench_Config {
        /** Number of steady-state samples requested for each benchmark. */
        std::size_t iterations = DEFAULT_ITERATIONS;
        /** Application payload bytes carried per logical message. */
        std::size_t payload_size = DEFAULT_PAYLOAD_SIZE;
    };

    /** @brief Parses optional `[iterations] [payload_size_bytes]` positional arguments. */
    [[nodiscard]] auto Parse_Args( int argc, char** argv ) -> Bench_Config {
        const std::span<char*> args( argv, static_cast<std::size_t>( argc ) );
        Bench_Config config;

        if( argc > 1 ) {
            config.iterations = static_cast<std::size_t>( std::stoul( args[1] ) );
        }
        if( argc > 2 ) {
            config.payload_size = static_cast<std::size_t>( std::stoul( args[2] ) );
        }

        return config;
    }

    /** @brief Summary latency percentiles reported in microseconds. */
    struct Percentiles {
        /** Median sample latency in microseconds. */
        double p50_us;
        /** 95th-percentile sample latency in microseconds. */
        double p95_us;
        /** 99th-percentile sample latency in microseconds. */
        double p99_us;
    };

    /** @brief Sorts latency samples and extracts nearest-index p50/p95/p99 values. */
    [[nodiscard]] auto Compute_Percentiles( std::vector<double> samples_us ) -> Percentiles {
        if( samples_us.empty() ) {
            return Percentiles { 0.0, 0.0, 0.0 };
        }

        std::sort( samples_us.begin(), samples_us.end() );

        auto at_fraction = [&samples_us]( double fraction ) {
            const auto index = static_cast<std::size_t>( fraction * static_cast<double>( samples_us.size() - 1 ) );
            return samples_us[index];
        };

        return Percentiles { at_fraction( P50_FRACTION ), at_fraction( P95_FRACTION ), at_fraction( P99_FRACTION ) };
    }

    /** @brief Emits one human-readable percentile summary line. */
    void Print_Percentiles( std::string_view label, const Percentiles& percentiles, std::size_t sample_count ) {
        std::cout << label << " (" << sample_count << " samples): p50=" << percentiles.p50_us
                  << "us p95=" << percentiles.p95_us << "us p99=" << percentiles.p99_us << "us\n";
    }

    /**
     * @brief Measures synchronous REQ/REP round-trip latency.
     * @param endpoint Unique local IPC endpoint used for the measurement.
     * @param config Iteration count and payload size.
     * @return Successful round-trip samples in microseconds; failed samples are diagnosed and skipped.
     * @details The Replier owns a dedicated thread while Requester remains on the caller thread,
     * preserving ZeroMQ socket affinity and approximating the service adapter topology.
     */
    [[nodiscard]] auto Measure_Req_Rep( const std::string& endpoint, const Bench_Config& config )
        -> std::vector<double> {
        Context requester_context;
        auto requester_result = Requester::Connect( requester_context, endpoint );
        if( !requester_result ) {
            std::cerr << "edge_ipc_bench: failed to create Requester\n";
            return {};
        }
        auto& requester = *requester_result;

        const std::string request_payload( config.payload_size, 'r' );
        const std::string reply_payload( config.payload_size, 'p' );

        std::jthread replier_thread( [&]( const std::stop_token& stop_token ) {
            Context replier_context;
            auto replier_result = Replier::Bind( replier_context, endpoint );
            if( !replier_result ) {
                return;
            }
            auto& replier = *replier_result;

            while( !stop_token.stop_requested() ) {
                auto receive_result = replier.Receive( REPLIER_POLL_TIMEOUT );
                if( !receive_result || !receive_result->has_value() ) {
                    continue;
                }
                std::ignore = ( *receive_result )->Reply( reply_payload );
            }
        } );

        // Wait for the connection to be established (bounded retry, never a fixed sleep).
        bool connected = false;
        const auto connect_deadline = std::chrono::steady_clock::now() + CONNECT_DEADLINE;
        while( !connected && std::chrono::steady_clock::now() < connect_deadline ) {
            connected = requester.Request( request_payload, PER_ATTEMPT_TIMEOUT ).has_value();
        }

        std::vector<double> latencies_us;
        if( !connected ) {
            std::cerr << "edge_ipc_bench: failed to establish REQ/REP connection\n";
        } else {
            latencies_us.reserve( config.iterations );
            for( std::size_t i = 0; i < config.iterations; ++i ) {
                const auto start = std::chrono::steady_clock::now();
                auto result = requester.Request( request_payload, STEADY_STATE_TIMEOUT );
                const auto end = std::chrono::steady_clock::now();

                if( !result ) {
                    std::cerr << "edge_ipc_bench: request " << i << " failed, skipping sample\n";
                    continue;
                }

                latencies_us.push_back( std::chrono::duration<double, std::micro>( end - start ).count() );
            }
        }

        replier_thread.request_stop();
        replier_thread.join();
        return latencies_us;
    }

    /**
     * @brief Measures one-way PUB/SUB delivery latency using an embedded steady-clock timestamp.
     * @param endpoint Unique local IPC endpoint used for the measurement.
     * @param config Iteration count and requested payload size.
     * @return Successful one-way samples in microseconds.
     * @details Both sockets remain on the caller thread; the send timestamp travels in the payload so
     * the measurement does not rely on cross-thread/process clock synchronization. Slow-joiner setup
     * uses bounded retries rather than a fixed startup sleep.
     */
    [[nodiscard]] auto Measure_Pub_Sub( const std::string& endpoint, const Bench_Config& config )
        -> std::vector<double> {
        Context context;
        auto publisher_result = Publisher::Bind( context, endpoint );
        auto subscriber_result = Subscriber::Connect( context, endpoint, "bench" );
        if( !publisher_result || !subscriber_result ) {
            std::cerr << "edge_ipc_bench: failed to create Publisher/Subscriber\n";
            return {};
        }
        auto& publisher = *publisher_result;
        auto& subscriber = *subscriber_result;

        const std::size_t effective_payload_size = std::max( config.payload_size, sizeof( std::int64_t ) );
        std::string payload( effective_payload_size, 'x' );

        auto publish_with_timestamp = [&]() -> bool {
            const auto send_time = std::chrono::steady_clock::now();
            const std::int64_t ticks = send_time.time_since_epoch().count();
            std::memcpy( payload.data(), &ticks, sizeof( ticks ) );
            return publisher.Publish( "bench", payload ).has_value();
        };

        // Slow joiner: retry publishing until the subscriber observes a message, bounded by a deadline.
        bool connected = false;
        const auto connect_deadline = std::chrono::steady_clock::now() + CONNECT_DEADLINE;
        while( !connected && std::chrono::steady_clock::now() < connect_deadline ) {
            if( !publish_with_timestamp() ) {
                continue;
            }
            auto receive_result = subscriber.Receive( PER_ATTEMPT_TIMEOUT );
            connected = receive_result.has_value() && receive_result->has_value();
        }

        std::vector<double> latencies_us;
        if( !connected ) {
            std::cerr << "edge_ipc_bench: failed to establish PUB/SUB connection\n";
            return latencies_us;
        }

        latencies_us.reserve( config.iterations );
        for( std::size_t i = 0; i < config.iterations; ++i ) {
            if( !publish_with_timestamp() ) {
                std::cerr << "edge_ipc_bench: publish " << i << " failed, skipping sample\n";
                continue;
            }

            auto receive_result = subscriber.Receive( STEADY_STATE_TIMEOUT );
            const auto receive_time = std::chrono::steady_clock::now();
            if( !receive_result ) {
                std::cerr << "edge_ipc_bench: receive " << i << " failed, skipping sample\n";
                continue;
            }
            std::optional<Message>& maybe_message = receive_result.value();
            if( !maybe_message.has_value() ) {
                std::cerr << "edge_ipc_bench: receive " << i << " timed out, skipping sample\n";
                continue;
            }
            const Message& message = maybe_message.value();

            std::int64_t sent_ticks = 0;
            std::memcpy( &sent_ticks, message.payload.data(), sizeof( sent_ticks ) );
            const auto send_time =
                std::chrono::steady_clock::time_point( std::chrono::steady_clock::duration( sent_ticks ) );

            latencies_us.push_back( std::chrono::duration<double, std::micro>( receive_time - send_time ).count() );
        }

        return latencies_us;
    }

}  // namespace

/**
 * @brief Runs both IPC latency measurements and prints percentile summaries.
 * @return 0 on benchmark completion, 1 on fatal configuration/runtime exception.
 */
auto main( int argc, char** argv ) -> int {
    try {
        const Bench_Config config = Parse_Args( argc, argv );

        std::cout << "edge_ipc_bench: iterations=" << config.iterations << " payload_size=" << config.payload_size
                  << " bytes\n";
        std::cout << "DISCLAIMER: these are development-container measurements on shared build hardware. "
                     "They are NOT evidence for the production sub-millisecond IPC latency target; that claim "
                     "requires representative target-hardware measurement (see .github/skills/zeromq-ipc/SKILL.md).\n";

        const edge::test::Temp_Dir temp_dir;
        const std::string reqrep_endpoint = "ipc://" + temp_dir.Path() + "/reqrep.ipc";
        const std::string pubsub_endpoint = "ipc://" + temp_dir.Path() + "/pubsub.ipc";

        const auto reqrep_latencies_us = Measure_Req_Rep( reqrep_endpoint, config );
        Print_Percentiles(
            "REQ/REP round-trip", Compute_Percentiles( reqrep_latencies_us ), reqrep_latencies_us.size() );

        const auto pubsub_latencies_us = Measure_Pub_Sub( pubsub_endpoint, config );
        Print_Percentiles( "PUB/SUB one-way", Compute_Percentiles( pubsub_latencies_us ), pubsub_latencies_us.size() );

        return 0;
    } catch( const std::exception& error ) {
        std::cerr << "edge_ipc_bench: fatal error: " << error.what() << '\n';
        return 1;
    }
}
