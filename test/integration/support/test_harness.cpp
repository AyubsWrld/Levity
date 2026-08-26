/**
 * @file test_harness.cpp
 * @brief Test implementation for test harness behavior and regression coverage.
 *
 * @details Source path: `test/integration/support/test_harness.cpp`.
 */

#include "test_harness.h"

#include "deterministic_targets.h"
#include "poll_until.h"

#include <arpa/inet.h>
#include <edge/platform/posix/unique_fd.h>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

// The three executable paths are injected as compile definitions by test/integration/CMakeLists.txt
// (`$<TARGET_FILE:...>`), never hardcoded, so the suite tracks wherever CMake actually placed the
// build artifacts.
#ifndef EDGE_API_GATEWAY_BINARY
#error "EDGE_API_GATEWAY_BINARY must be supplied by test/integration/CMakeLists.txt"
#endif
#ifndef EDGE_GIMBAL_CONTROLLER_BINARY
#error "EDGE_GIMBAL_CONTROLLER_BINARY must be supplied by test/integration/CMakeLists.txt"
#endif
#ifndef EDGE_DUMMY_MONOLITH_BINARY
#error "EDGE_DUMMY_MONOLITH_BINARY must be supplied by test/integration/CMakeLists.txt"
#endif

namespace edge::integration_test::support {
    namespace {

        constexpr auto READINESS_POLL_BACKOFF = std::chrono::milliseconds( 20 );
        // dummy_monolith's own default is 200ms; the harness runs it faster so bounded readiness
        // polling (slow-joiner PUB/SUB included) converges quickly without ever depending on
        // receiving any particular publish cycle.
        constexpr const char* HARNESS_TELEMETRY_INTERVAL_MS = "50";
        /** Bounded per-request client timeouts, generous enough to comfortably observe the
         * documented ~1000 ms GIMBAL_REQUEST_TIMEOUT_MS round trip, but never unbounded. */
        constexpr int CONNECTION_TIMEOUT_SECONDS = 2;
        constexpr int READ_TIMEOUT_SECONDS = 5;
        constexpr int WRITE_TIMEOUT_SECONDS = 2;
        /** HTTP status code expected from a successful readiness/cues probe. */
        constexpr int HTTP_OK = 200;

        /** @brief Applies child-environment overrides by variable name without duplicating keys. */
        void Apply_Overrides( std::vector<Process_Handle::Environment_Variable>& base,
                              const std::vector<Process_Handle::Environment_Variable>& overrides ) {
            for( const auto& override_variable : overrides ) {
                const auto existing = std::find_if( base.begin(), base.end(), [&]( const auto& variable ) {
                    return variable.name == override_variable.name;
                } );
                if( existing != base.end() ) {
                    existing->value = override_variable.value;
                } else {
                    base.push_back( override_variable );
                }
            }
        }

        // Strips the "ipc://" scheme prefix from an endpoint to get the underlying filesystem path.
        /** @brief Extracts the filesystem path from a known `ipc://` integration endpoint. */
        [[nodiscard]] auto Socket_Path_From_Endpoint( const std::string& endpoint ) -> std::string {
            constexpr std::string_view SCHEME = "ipc://";
            if( endpoint.rfind( SCHEME, 0 ) == 0 ) {
                return endpoint.substr( SCHEME.size() );
            }
            return endpoint;
        }

        /** @brief Adds one process role's live/retained output to aggregate diagnostics. */
        void Dump_Role( std::ostringstream& out,
                        const char* label,
                        std::optional<Process_Handle>& handle,
                        const std::string& last_output ) {
            out << "----- " << label << " -----\n";
            if( handle.has_value() ) {
                out << "(pid=" << handle->Pid() << ", alive=" << ( handle->Is_Alive() ? "yes" : "no" ) << ")\n";
                out << handle->Drain_Output();
            } else if( !last_output.empty() ) {
                out << "(not currently running; last captured output below)\n" << last_output;
            } else {
                out << "(never started)\n";
            }
        }

    }  // namespace

    /** @copydoc Allocate_Free_Tcp_Port() */
    auto Allocate_Free_Tcp_Port() -> std::uint16_t {
        const edge::platform::posix::Unique_Fd probe_fd { ::socket( AF_INET, SOCK_STREAM, 0 ) };
        if( !probe_fd ) {
            throw std::runtime_error( "Allocate_Free_Tcp_Port: socket() failed" );
        }

        sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
        address.sin_port = 0;

        if( ::bind( probe_fd.Get(),
                    reinterpret_cast<sockaddr*>( &address ),
                    sizeof( address ) ) !=  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            0 ) {
            throw std::runtime_error( "Allocate_Free_Tcp_Port: bind() failed" );
        }

        socklen_t address_length = sizeof( address );
        if( ::getsockname( probe_fd.Get(),
                           reinterpret_cast<sockaddr*>( &address ),
                           &address_length ) !=  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            0 ) {
            throw std::runtime_error( "Allocate_Free_Tcp_Port: getsockname() failed" );
        }

        const auto port = ntohs( address.sin_port );
        return port;
    }

    /** @copydoc Test_Harness::Test_Harness */
    Test_Harness::Test_Harness()
        : m_telemetry_endpoint( "ipc://" + m_temp_dir.Path() + "/telemetry.ipc" ),
          m_gimbal_endpoint( "ipc://" + m_temp_dir.Path() + "/gimbal-cmd.ipc" ),
          m_http_port( Allocate_Free_Tcp_Port() ),
          m_client( "127.0.0.1", m_http_port ) {
        // Bounded per-request timeouts: generous enough to comfortably observe the documented
        // ~1000 ms GIMBAL_REQUEST_TIMEOUT_MS round trip, but never unbounded.
        m_client.set_connection_timeout( CONNECTION_TIMEOUT_SECONDS, 0 );
        m_client.set_read_timeout( READ_TIMEOUT_SECONDS, 0 );
        m_client.set_write_timeout( WRITE_TIMEOUT_SECONDS, 0 );
    }

    /** @copydoc Test_Harness::~Test_Harness */
    Test_Harness::~Test_Harness() {
        // Unconditional teardown regardless of test outcome. Process_Handle's own destructor is a
        // second, independent safety net if any of these were somehow skipped.
        Stop_Gateway();
        Stop_Gimbal();
        Stop_Monolith();
    }

    /** @copydoc Test_Harness::Telemetry_Endpoint */
    auto Test_Harness::Telemetry_Endpoint() const -> const std::string& { return m_telemetry_endpoint; }

    /** @copydoc Test_Harness::Gimbal_Endpoint */
    auto Test_Harness::Gimbal_Endpoint() const -> const std::string& { return m_gimbal_endpoint; }

    /** @copydoc Test_Harness::Telemetry_Socket_Path */
    auto Test_Harness::Telemetry_Socket_Path() const -> std::string {
        return Socket_Path_From_Endpoint( m_telemetry_endpoint );
    }

    /** @copydoc Test_Harness::Gimbal_Socket_Path */
    auto Test_Harness::Gimbal_Socket_Path() const -> std::string {
        return Socket_Path_From_Endpoint( m_gimbal_endpoint );
    }

    /** @copydoc Test_Harness::Http_Port */
    auto Test_Harness::Http_Port() const noexcept -> std::uint16_t { return m_http_port; }

    /** @copydoc Test_Harness::Udp */
    auto Test_Harness::Udp() -> Udp_Listener& { return m_udp_listener; }

    /** @copydoc Test_Harness::Start_Gimbal */
    void Test_Harness::Start_Gimbal( const std::vector<Process_Handle::Environment_Variable>& extra_environment ) {
        std::vector<Process_Handle::Environment_Variable> environment {
            { "EDGE_GIMBAL_COMMAND_ENDPOINT", m_gimbal_endpoint },
            { "EDGE_STANAG_UDP_HOST", "127.0.0.1" },
            { "EDGE_STANAG_UDP_PORT", std::to_string( m_udp_listener.Port() ) },
        };
        Apply_Overrides( environment, extra_environment );
        m_gimbal.emplace( EDGE_GIMBAL_CONTROLLER_BINARY, std::move( environment ), "gimbal_controller" );
    }

    /** @copydoc Test_Harness::Start_Gateway */
    void Test_Harness::Start_Gateway( const std::vector<Process_Handle::Environment_Variable>& extra_environment ) {
        std::vector<Process_Handle::Environment_Variable> environment {
            { "EDGE_TELEMETRY_ENDPOINT", m_telemetry_endpoint },
            { "EDGE_GIMBAL_COMMAND_ENDPOINT", m_gimbal_endpoint },
            { "EDGE_HTTP_HOST", "127.0.0.1" },
            { "EDGE_HTTP_PORT", std::to_string( m_http_port ) },
        };
        Apply_Overrides( environment, extra_environment );
        m_gateway.emplace( EDGE_API_GATEWAY_BINARY, std::move( environment ), "api_gateway" );
    }

    /** @copydoc Test_Harness::Start_Monolith */
    void Test_Harness::Start_Monolith( const std::vector<Process_Handle::Environment_Variable>& extra_environment ) {
        std::vector<Process_Handle::Environment_Variable> environment {
            { "EDGE_TELEMETRY_ENDPOINT", m_telemetry_endpoint },
            { "EDGE_TELEMETRY_INTERVAL_MS", HARNESS_TELEMETRY_INTERVAL_MS },
        };
        Apply_Overrides( environment, extra_environment );
        m_monolith.emplace( EDGE_DUMMY_MONOLITH_BINARY, std::move( environment ), "dummy_monolith" );
    }

    /** @copydoc Test_Harness::Stop_Gimbal */
    void Test_Harness::Stop_Gimbal() {
        if( m_gimbal.has_value() ) {
            m_gimbal->Terminate();
            m_last_gimbal_output = m_gimbal->Drain_Output();
            m_gimbal.reset();
        }
    }

    /** @copydoc Test_Harness::Stop_Gateway */
    void Test_Harness::Stop_Gateway() {
        if( m_gateway.has_value() ) {
            m_gateway->Terminate();
            m_last_gateway_output = m_gateway->Drain_Output();
            m_gateway.reset();
        }
    }

    /** @copydoc Test_Harness::Stop_Monolith */
    void Test_Harness::Stop_Monolith() {
        if( m_monolith.has_value() ) {
            m_monolith->Terminate();
            m_last_monolith_output = m_monolith->Drain_Output();
            m_monolith.reset();
        }
    }

    /** @copydoc Test_Harness::Kill_Monolith */
    void Test_Harness::Kill_Monolith() {
        if( m_monolith.has_value() ) {
            m_monolith->Kill();
            m_last_monolith_output = m_monolith->Drain_Output();
            m_monolith.reset();
        }
    }

    /** @copydoc Test_Harness::Gimbal_Alive */
    auto Test_Harness::Gimbal_Alive() -> bool { return m_gimbal.has_value() && m_gimbal->Is_Alive(); }

    /** @copydoc Test_Harness::Gateway_Alive */
    auto Test_Harness::Gateway_Alive() -> bool { return m_gateway.has_value() && m_gateway->Is_Alive(); }

    /** @copydoc Test_Harness::Monolith_Alive */
    auto Test_Harness::Monolith_Alive() -> bool { return m_monolith.has_value() && m_monolith->Is_Alive(); }

    /** @copydoc Test_Harness::Unlink_Telemetry_Socket_File */
    void Test_Harness::Unlink_Telemetry_Socket_File() const {
        std::error_code ec;
        std::filesystem::remove( Telemetry_Socket_Path(), ec );
    }

    /** @copydoc Test_Harness::Unlink_Gimbal_Socket_File */
    void Test_Harness::Unlink_Gimbal_Socket_File() const {
        std::error_code ec;
        std::filesystem::remove( Gimbal_Socket_Path(), ec );
    }

    /** @copydoc Test_Harness::Wait_For_Gimbal_Ready */
    auto Test_Harness::Wait_For_Gimbal_Ready( std::chrono::milliseconds timeout ) -> bool {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        return Poll_Until( deadline, READINESS_POLL_BACKOFF, [&] {
            return Gimbal_Alive() && std::filesystem::exists( Gimbal_Socket_Path() );
        } );
    }

    /** @copydoc Test_Harness::Wait_For_Gateway_Ready */
    auto Test_Harness::Wait_For_Gateway_Ready( std::chrono::milliseconds timeout ) -> bool {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        return Poll_Until( deadline, READINESS_POLL_BACKOFF, [&] {
            if( !Gateway_Alive() ) {
                return false;
            }
            const auto response = Get( "/api/v1/ready" );
            return static_cast<bool>( response ) && response->status == HTTP_OK;
        } );
    }

    /** @copydoc Test_Harness::Cues_Match */
    auto Test_Harness::Cues_Match( std::span<const std::string_view> expected_uids ) -> bool {
        if( !Gateway_Alive() ) {
            return false;
        }
        const auto response = Get( "/api/v1/cues" );
        if( !response || response->status != HTTP_OK ) {
            return false;
        }
        try {
            const auto body = nlohmann::json::parse( response->body );
            const auto& targets = body.at( "targets" );
            if( targets.size() != expected_uids.size() ) {
                return false;
            }
            for( std::size_t index = 0; index < expected_uids.size(); ++index ) {
                if( targets.at( index ).at( "uid" ).get<std::string>() != expected_uids[index] ) {
                    return false;
                }
            }
            return true;
        } catch( const nlohmann::json::exception& ) {
            return false;
        }
    }

    /** @copydoc Test_Harness::Wait_For_Deterministic_Telemetry */
    auto Test_Harness::Wait_For_Deterministic_Telemetry( std::chrono::milliseconds timeout ) -> bool {
        std::array<std::string_view, DETERMINISTIC_TARGETS.size()> expected_uids {};
        for( std::size_t index = 0; index < DETERMINISTIC_TARGETS.size(); ++index ) {
            expected_uids.at( index ) = DETERMINISTIC_TARGETS.at( index ).uid;
        }

        const auto deadline = std::chrono::steady_clock::now() + timeout;
        return Poll_Until( deadline, READINESS_POLL_BACKOFF, [&] { return Cues_Match( expected_uids ); } );
    }

    /** @copydoc Test_Harness::Gateway_Output_Contains */
    auto Test_Harness::Gateway_Output_Contains( std::string_view text ) -> bool {
        return m_gateway.has_value() && m_gateway->Drain_Output().find( text ) != std::string::npos;
    }

    /** @copydoc Test_Harness::Diagnostics */
    auto Test_Harness::Diagnostics() -> std::string {
        std::ostringstream out;
        out << "\n=== harness diagnostics (temp_dir=" << m_temp_dir.Path() << ", http_port=" << m_http_port
            << ", udp_port=" << m_udp_listener.Port() << ") ===\n";
        Dump_Role( out, "gimbal_controller", m_gimbal, m_last_gimbal_output );
        Dump_Role( out, "api_gateway", m_gateway, m_last_gateway_output );
        Dump_Role( out, "dummy_monolith", m_monolith, m_last_monolith_output );
        out << "=== end harness diagnostics ===\n";
        return out.str();
    }

    /** @copydoc Test_Harness::Get */
    auto Test_Harness::Get( const std::string& path ) -> httplib::Result { return m_client.Get( path ); }

    /** @copydoc Test_Harness::Post */
    auto Test_Harness::Post( const std::string& path, const std::string& body, const std::string& content_type )
        -> httplib::Result {
        return m_client.Post( path, body, content_type );
    }

}  // namespace edge::integration_test::support
