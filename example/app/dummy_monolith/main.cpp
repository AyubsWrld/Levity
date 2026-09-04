// dummy_monolith: intentionally flat Tier 1 bootstrap telemetry publisher.
//
// This executable deliberately has no hexagonal layers (see .github/bootstrap-mvp.md Phase 3): it
// exists to give the API Gateway's inbound telemetry adapter a real PUB peer to integrate against,
// not to model a production monolith's internal architecture.
//
// Contract references: docs/architecture/tier1-contracts.md §4 (CuePriorityList ordering is
// preserved verbatim, never reordered/ranked), §11 (PUB/SUB framing is two frames,
// `[topic][payload]`) and §12 (configuration surface / defaults).
#include "edge/ipc/context.h"
#include "edge/ipc/endpoint.h"
#include "edge/ipc/publisher.h"
#include "telemetry.pb.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

namespace {

    using edge::ipc::Default_Telemetry_Endpoint;
    using edge::ipc::Endpoint_From_Environment;
    using edge::ipc::Ensure_Endpoint_Directory;
    using edge::ipc::Ipc_Context;
    using edge::ipc::Publisher;

    constexpr std::string_view SERVICE_NAME = "dummy_monolith";
    constexpr std::string_view TELEMETRY_TOPIC = "cues";
    constexpr int DEFAULT_TELEMETRY_INTERVAL_MS = 200;

    // The publish loop must observe the shutdown flag at least this often even when the configured
    // publish interval is much longer, so termination stays prompt.
    constexpr auto MAX_WAIT_SLICE = std::chrono::milliseconds( 100 );

    // Async-signal-safe shutdown flag: the signal handler only ever writes this
    // `volatile std::sig_atomic_t` and does nothing else, per signal-handler safety rules. It must
    // be a mutable global: async-signal-context code cannot use encapsulated/locked state.
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    volatile std::sig_atomic_t g_stop_requested = 0;

    extern "C" void Handle_Shutdown_Signal( int /*signal_number*/ ) { g_stop_requested = 1; }

    void Install_Signal_Handlers() {
        std::signal( SIGINT, Handle_Shutdown_Signal );
        std::signal( SIGTERM, Handle_Shutdown_Signal );
    }

    [[nodiscard]] auto Read_Telemetry_Interval_Ms() -> int {
        const char* raw_value = std::getenv( "EDGE_TELEMETRY_INTERVAL_MS" );
        if( raw_value == nullptr || *raw_value == '\0' ) {
            return DEFAULT_TELEMETRY_INTERVAL_MS;
        }

        int parsed = 0;
        try {
            parsed = std::stoi( raw_value );
        } catch( const std::exception& ) {
            return DEFAULT_TELEMETRY_INTERVAL_MS;
        }

        // A non-positive interval would make `Interruptible_Sleep`'s `remaining.count() > 0` guard
        // false immediately, turning the publish loop into an unpaced busy loop that pegs a CPU on
        // SWaP-constrained hardware. Treat it exactly like an unparseable value: fail safe to the
        // default rather than honouring it.
        if( parsed <= 0 ) {
            return DEFAULT_TELEMETRY_INTERVAL_MS;
        }
        return parsed;
    }

    struct Deterministic_Target {
        std::string_view uid;
        double latitude_deg;
        double longitude_deg;
        double altitude_m;
    };

    // Fixed, deterministic Tier 1 target set (tier1-contracts.md §4): the same three targets, in
    // the same order, on every publish cycle. Integration tests depend on this determinism.
    constexpr std::array<Deterministic_Target, 3> DETERMINISTIC_TARGETS = { {
        { "drone_01", 34.0500, -118.2500, 150.0 },
        { "drone_02", 34.0600, -118.2600, 220.5 },
        { "ground_01", 34.0400, -118.2400, 12.0 },
    } };

    [[nodiscard]] auto Build_Deterministic_Cue_List() -> edge::telemetry::v1::CuePriorityList {
        edge::telemetry::v1::CuePriorityList cue_list;

        for( const auto& target : DETERMINISTIC_TARGETS ) {
            auto* entry = cue_list.add_targets();
            entry->set_uid( std::string( target.uid ) );
            entry->set_latitude_deg( target.latitude_deg );
            entry->set_longitude_deg( target.longitude_deg );
            entry->set_altitude_m( target.altitude_m );
        }

        return cue_list;
    }

    // Sleeps for `duration`, waking at least every `MAX_WAIT_SLICE` to observe the stop flag. This
    // is pacing for the publish interval, not a correctness/synchronization sleep.
    void Interruptible_Sleep( std::chrono::milliseconds duration ) {
        auto remaining = duration;
        while( remaining.count() > 0 && g_stop_requested == 0 ) {
            const auto slice = std::min( remaining, MAX_WAIT_SLICE );
            std::this_thread::sleep_for( slice );
            remaining -= slice;
        }
    }

}  // namespace

auto main() -> int {
    try {
        Install_Signal_Handlers();

        const std::string endpoint =
            Endpoint_From_Environment( "EDGE_TELEMETRY_ENDPOINT", Default_Telemetry_Endpoint() );
        const int interval_ms = Read_Telemetry_Interval_Ms();

        // The context is created first and, as a local in main(), destroyed last (after `publisher`
        // below goes out of scope first in reverse declaration order).
        Ipc_Context context;

        if( const auto directory_result = Ensure_Endpoint_Directory( endpoint ); !directory_result.has_value() ) {
            std::cerr << "dummy_monolith: startup failed: could not prepare IPC directory for endpoint='" << endpoint
                      << "' detail='" << directory_result.error().detail << "'\n";
            return 1;
        }

        auto publisher_result = Publisher::Create( context, endpoint );
        if( !publisher_result.has_value() ) {
            std::cerr << "dummy_monolith: startup failed: could not bind telemetry PUB socket endpoint='" << endpoint
                      << "' detail='" << publisher_result.error().detail << "'\n";
            return 1;
        }
        auto& publisher = *publisher_result;

        std::cout << "dummy_monolith: startup service=" << SERVICE_NAME << " endpoint=" << endpoint
                  << " interval_ms=" << interval_ms << '\n';

        const edge::telemetry::v1::CuePriorityList cue_list = Build_Deterministic_Cue_List();
        std::string serialized_payload;
        if( !cue_list.SerializeToString( &serialized_payload ) ) {
            std::cerr << "dummy_monolith: startup failed: could not serialize the deterministic CuePriorityList\n";
            return 1;
        }

        while( g_stop_requested == 0 ) {
            if( const auto publish_result = publisher.Publish( TELEMETRY_TOPIC, serialized_payload );
                !publish_result.has_value() ) {
                std::cerr << "dummy_monolith: publish failed detail='" << publish_result.error().detail << "'\n";
            }

            Interruptible_Sleep( std::chrono::milliseconds( interval_ms ) );
        }

        std::cout << "dummy_monolith: shutdown service=" << SERVICE_NAME << '\n';

        return 0;
    } catch( const std::exception& error ) {
        std::cerr << "dummy_monolith: fatal error: " << error.what() << '\n';
        return 1;
    } catch( ... ) {
        std::cerr << "dummy_monolith: fatal error: non-standard exception\n";
        return 1;
    }
}
