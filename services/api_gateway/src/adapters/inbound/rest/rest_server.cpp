/**
 * @file rest_server.cpp
 * @brief Implementation of rest server.
 *
 * @details Source path: `services/api_gateway/src/adapters/inbound/rest/rest_server.cpp`.
 */

#include <edge/api_gateway/adapters/inbound/rest/rest_server.h>
#include <httplib/httplib.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace edge::api_gateway::adapters::inbound::rest {

    namespace {

        /** HTTP status codes emitted by this adapter's response mapping. */
        constexpr int HTTP_OK = 200;
        constexpr int HTTP_BAD_REQUEST = 400;
        constexpr int HTTP_NOT_FOUND = 404;
        constexpr int HTTP_CONFLICT = 409;
        constexpr int HTTP_PAYLOAD_TOO_LARGE = 413;
        constexpr int HTTP_INTERNAL_SERVER_ERROR = 500;
        constexpr int HTTP_SERVICE_UNAVAILABLE = 503;
        constexpr int HTTP_GATEWAY_TIMEOUT = 504;

        /** @brief Writes a JSON body and status code using the adapter's canonical content type. */
        void Set_Json_Response( httplib::Response& response, int status, const nlohmann::json& body ) {
            response.status = status;
            response.set_content( body.dump(), "application/json" );
        }

        /** @brief Maps core slew-failure reasons to stable REST/logging error tokens. */
        [[nodiscard]] auto Classify_Slew_Failure( core::ports::inbound::Slew_Failure_Reason reason )
            -> std::string_view {
            using core::ports::inbound::Slew_Failure_Reason;

            switch( reason ) {
                case Slew_Failure_Reason::UNKNOWN_TARGET:
                    return "unknown_target";
                case Slew_Failure_Reason::GIMBAL_REJECTED:
                    return "gimbal_rejected";
                case Slew_Failure_Reason::GIMBAL_HARDWARE_ERROR:
                    return "gimbal_hardware_error";
                case Slew_Failure_Reason::GIMBAL_UNAVAILABLE:
                case Slew_Failure_Reason::GIMBAL_QUEUE_FULL:
                case Slew_Failure_Reason::GIMBAL_TRANSPORT_ERROR:
                    return "gimbal_unavailable";
                case Slew_Failure_Reason::GIMBAL_TIMEOUT:
                    return "gimbal_timeout";
            }

            return "unknown_failure";
        }

    }  // namespace

    /** @brief Private HTTP server state and non-owning application/logger dependencies. */
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes,cppcoreguidelines-avoid-const-or-ref-data-members)
    // Impl is a private pimpl-style aggregate: it is declared only in this translation unit and
    // accessed solely through the owning Rest_Server and the request-handler lambdas below, so
    // there is no external caller to encapsulate against. The Api_Port/Logger reference members are
    // non-owning dependencies documented (and guaranteed to outlive Rest_Server) by the runtime
    // composition root, matching the reference-member pattern used by the core services.
    struct Rest_Server::Impl {
        /** Non-owning application port invoked by request handlers. */
        core::ports::inbound::Api_Port& api;
        /** Non-owning logger used for HTTP boundary diagnostics. */
        edge::platform::logging::Logger& logger;
        /** Owned cpp-httplib server/listener implementation. */
        httplib::Server server;
        /** Thread running `listen_after_bind()` until Stop() closes the server. */
        std::jthread listener;
        /** Actual TCP port bound by the listener. */
        std::uint16_t bound_port = 0;
        /** Idempotence guard preventing duplicate stop/close sequences. */
        std::atomic<bool> stopped { false };

        Impl( core::ports::inbound::Api_Port& api_ref, edge::platform::logging::Logger& logger_ref )
            : api( api_ref ), logger( logger_ref ) {}
    };
    // NOLINTEND(misc-non-private-member-variables-in-classes,cppcoreguidelines-avoid-const-or-ref-data-members)

    /** @copydoc Rest_Server::Rest_Server */
    Rest_Server::Rest_Server( std::string host,
                              std::uint16_t port,
                              core::ports::inbound::Api_Port& api,
                              edge::platform::logging::Logger& logger )
        : m_impl( std::make_unique<Impl>( api, logger ) ) {
        m_impl->server.set_payload_max_length( MAX_REST_BODY_BYTES );
        m_impl->server.new_task_queue = [] {
            // cpp-httplib's TaskQueue factory signature requires a raw owning pointer that the
            // library itself takes ownership of and destroys; there is no smart-pointer-returning
            // overload to conform to instead.
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            return new httplib::ThreadPool( REST_WORKER_THREADS, MAX_QUEUED_REQUESTS );
        };

        m_impl->server.Get(
            "/api/v1/cues", [impl = m_impl.get()]( const httplib::Request&, httplib::Response& response ) {
                const auto snapshot = impl->api.Get_Cues();

                auto targets = nlohmann::json::array();
                for( const auto& target : snapshot.targets() ) {
                    targets.push_back( {
                        { "uid", target.uid() },
                        { "latitude_deg", target.latitude_deg() },
                        { "longitude_deg", target.longitude_deg() },
                        { "altitude_m", target.altitude_m() },
                    } );
                }

                Set_Json_Response( response, HTTP_OK, { { "targets", targets } } );
                impl->logger.Info( "http_request",
                                   { { "method", "GET" }, { "path", "/api/v1/cues" }, { "result", "ok" } } );
            } );

        m_impl->server.Get(
            "/api/v1/ready", [impl = m_impl.get()]( const httplib::Request&, httplib::Response& response ) {
                Set_Json_Response( response, HTTP_OK, { { "status", "ready" } } );
                impl->logger.Info( "http_request",
                                   { { "method", "GET" }, { "path", "/api/v1/ready" }, { "result", "ok" } } );
            } );

        m_impl->server.Post(
            "/api/v1/slew", [impl = m_impl.get()]( const httplib::Request& request, httplib::Response& response ) {
                try {
                    const auto parsed = nlohmann::json::parse( request.body, nullptr, false );
                    if( parsed.is_discarded() || !parsed.is_object() ) {
                        Set_Json_Response(
                            response,
                            HTTP_BAD_REQUEST,
                            { { "error", "invalid_request" }, { "detail", "request body must be a JSON object" } } );
                        impl->logger.Warn(
                            "http_request",
                            { { "method", "POST" }, { "path", "/api/v1/slew" }, { "result", "invalid_request" } } );
                        return;
                    }

                    const auto uid_iterator = parsed.find( "target_uid" );
                    if( uid_iterator == parsed.end() || !uid_iterator->is_string() ||
                        uid_iterator->get<std::string>().empty() ) {
                        Set_Json_Response(
                            response,
                            HTTP_BAD_REQUEST,
                            { { "error", "invalid_request" }, { "detail", "target_uid must be a non-empty string" } } );
                        impl->logger.Warn(
                            "http_request",
                            { { "method", "POST" }, { "path", "/api/v1/slew" }, { "result", "invalid_request" } } );
                        return;
                    }

                    const std::string target_uid = uid_iterator->get<std::string>();
                    auto result = impl->api.Request_Slew( target_uid );
                    if( !result.has_value() ) {
                        const auto& failure = result.error();
                        using core::ports::inbound::Slew_Failure_Reason;

                        switch( failure.reason ) {
                            case Slew_Failure_Reason::UNKNOWN_TARGET:
                                Set_Json_Response( response,
                                                   HTTP_NOT_FOUND,
                                                   { { "error", "unknown_target" }, { "target_uid", target_uid } } );
                                break;
                            case Slew_Failure_Reason::GIMBAL_REJECTED:
                                Set_Json_Response( response, HTTP_CONFLICT, { { "error", "gimbal_rejected" } } );
                                break;
                            case Slew_Failure_Reason::GIMBAL_HARDWARE_ERROR:
                                Set_Json_Response(
                                    response, HTTP_SERVICE_UNAVAILABLE, { { "error", "gimbal_hardware_error" } } );
                                break;
                            case Slew_Failure_Reason::GIMBAL_UNAVAILABLE:
                            case Slew_Failure_Reason::GIMBAL_QUEUE_FULL:
                            case Slew_Failure_Reason::GIMBAL_TRANSPORT_ERROR:
                                Set_Json_Response(
                                    response, HTTP_SERVICE_UNAVAILABLE, { { "error", "gimbal_unavailable" } } );
                                break;
                            case Slew_Failure_Reason::GIMBAL_TIMEOUT:
                                Set_Json_Response( response,
                                                   HTTP_GATEWAY_TIMEOUT,
                                                   { { "error", "gimbal_timeout" }, { "outcome", "unknown" } } );
                                break;
                        }

                        impl->logger.Warn( "http_request",
                                           { { "method", "POST" },
                                             { "path", "/api/v1/slew" },
                                             { "target_uid", target_uid },
                                             { "result", std::string( Classify_Slew_Failure( failure.reason ) ) } } );
                        return;
                    }

                    Set_Json_Response(
                        response, HTTP_OK, { { "status", "accepted" }, { "target_uid", result->target_uid } } );
                    impl->logger.Info( "http_request",
                                       { { "method", "POST" },
                                         { "path", "/api/v1/slew" },
                                         { "target_uid", target_uid },
                                         { "result", "accepted" } } );
                } catch( const std::exception& error ) {
                    Set_Json_Response( response, HTTP_INTERNAL_SERVER_ERROR, { { "error", "internal_error" } } );
                    impl->logger.Error(
                        "http_request_exception",
                        { { "method", "POST" }, { "path", "/api/v1/slew" }, { "detail", error.what() } } );
                } catch( ... ) {
                    Set_Json_Response( response, HTTP_INTERNAL_SERVER_ERROR, { { "error", "internal_error" } } );
                    impl->logger.Error( "http_request_exception",
                                        { { "method", "POST" }, { "path", "/api/v1/slew" } } );
                }
            } );

        m_impl->server.set_error_handler(
            [impl = m_impl.get()]( const httplib::Request& request, httplib::Response& response ) {
                if( !response.body.empty() ) {
                    return;
                }

                if( response.status == HTTP_PAYLOAD_TOO_LARGE ) {
                    response.set_content( R"({"error":"body_too_large"})", "application/json" );
                } else {
                    response.status = HTTP_NOT_FOUND;
                    response.set_content( R"({"error":"not_found"})", "application/json" );
                }

                impl->logger.Warn( "http_request",
                                   { { "method", request.method },
                                     { "path", request.path },
                                     { "result", std::to_string( response.status ) } } );
            } );

        int bound_port = -1;
        if( port == 0 ) {
            bound_port = m_impl->server.bind_to_any_port( host );
        } else if( m_impl->server.bind_to_port( host, port ) ) {
            bound_port = port;
        }

        if( bound_port <= 0 ) {
            throw std::runtime_error( "Rest_Server: failed to bind to " + host + ":" + std::to_string( port ) );
        }
        m_impl->bound_port = static_cast<std::uint16_t>( bound_port );

        try {
            m_impl->listener = std::jthread( [impl = m_impl.get()] { impl->server.listen_after_bind(); } );
            m_impl->server.wait_until_ready();
            m_impl->logger.Info( "rest_server_started",
                                 { { "host", host }, { "port", std::to_string( m_impl->bound_port ) } } );
        } catch( ... ) {
            // If anything throws after the listener thread starts, this constructor will not reach
            // Rest_Server::~Rest_Server(). Stop/join explicitly so Impl's jthread destructor cannot
            // block forever waiting on a server that was never told to stop.
            m_impl->server.stop();
            if( m_impl->listener.joinable() ) {
                m_impl->listener.join();
            }
            throw;
        }
    }

    /** @copydoc Rest_Server::~Rest_Server */
    Rest_Server::~Rest_Server() { Stop(); }

    /** @copydoc Rest_Server::Bound_Port */
    auto Rest_Server::Bound_Port() const noexcept -> std::uint16_t { return m_impl->bound_port; }

    /** @copydoc Rest_Server::Stop */
    void Rest_Server::Stop() noexcept {
        if( !m_impl || m_impl->stopped.exchange( true ) ) {
            return;
        }

        m_impl->server.stop();
        if( m_impl->listener.joinable() ) {
            m_impl->listener.join();
        }
    }

}  // namespace edge::api_gateway::adapters::inbound::rest
