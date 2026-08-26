/**
 * @file endpoint.cpp
 * @brief Private implementation of endpoint; not part of the public module API.
 *
 * @details Source path: `shared/libs/ipc/src/detail/endpoint.cpp`.
 */

#include "detail/endpoint.h"

#include <sys/stat.h>

#include <cerrno>
#include <filesystem>
#include <ranges>
#include <string>
#include <system_error>
#include <vector>

namespace edge::ipc::detail {
    namespace {

        /** Endpoint scheme accepted by the current local-IPC-only library. */
        constexpr std::string_view IPC_SCHEME = "ipc://";

        /** @brief Builds a stable IPC Error from a filesystem operation and std::error_code. */
        [[nodiscard]] auto Filesystem_Error( Error_Code code,
                                             std::string_view action,
                                             const std::filesystem::path& path,
                                             const std::error_code& error ) -> Error {
            return Error { code, std::string( action ) + " '" + path.string() + "': " + error.message() };
        }

    }  // namespace

    /** @copydoc Validate_Endpoint() */
    auto Validate_Endpoint( std::string_view endpoint ) -> edge::Result<void, Error> {
        if( !endpoint.starts_with( IPC_SCHEME ) || endpoint.size() <= IPC_SCHEME.size() ) {
            return edge::Make_Failure(
                Error { Error_Code::INVALID_ENDPOINT, "endpoint must be a non-empty ipc:// endpoint" } );
        }
        return {};
    }

    /** @copydoc Prepare_Bind_Endpoint() */
    auto Prepare_Bind_Endpoint( std::string_view endpoint ) -> edge::Result<void, Error> {
        if( auto validation = Validate_Endpoint( endpoint ); !validation.has_value() ) {
            return validation;
        }

        const std::filesystem::path socket_path( endpoint.substr( IPC_SCHEME.size() ) );
        const auto parent = socket_path.parent_path();
        if( parent.empty() ) {
            return {};
        }

        // Discover which directories do not yet exist before creating anything. We only tighten
        // permissions on directories created by this call; a configured existing parent such as
        // /tmp must never have its permissions changed by the IPC library.
        std::vector<std::filesystem::path> missing_directories;
        auto cursor = parent;
        std::error_code error;
        while( !cursor.empty() ) {
            const bool exists = std::filesystem::exists( cursor, error );
            if( error ) {
                return edge::Make_Failure( Filesystem_Error(
                    Error_Code::BIND_FAILED, "failed to inspect IPC endpoint directory", cursor, error ) );
            }
            if( exists ) {
                if( !std::filesystem::is_directory( cursor, error ) ) {
                    if( error ) {
                        return edge::Make_Failure( Filesystem_Error(
                            Error_Code::BIND_FAILED, "failed to inspect IPC endpoint directory", cursor, error ) );
                    }
                    return edge::Make_Failure( Error {
                        Error_Code::BIND_FAILED,
                        "IPC endpoint parent is not a directory: '" + cursor.string() + "'",
                    } );
                }
                break;
            }

            missing_directories.push_back( cursor );
            const auto next = cursor.parent_path();
            if( next == cursor ) {
                break;
            }
            cursor = next;
        }

        if( missing_directories.empty() ) {
            return {};
        }

        // Create from the highest missing ancestor down to the configured parent. Use POSIX mkdir
        // with mode 0700 so a directory is private from the instant it becomes visible; creating it
        // with broad default permissions and chmod'ing afterward would introduce a short permission
        // window. EEXIST is treated as a concurrent-creator race and the existing path is validated
        // but never chmod'ed.
        for( const auto& directory : missing_directories | std::views::reverse ) {
            if( ::mkdir( directory.c_str(), S_IRWXU ) == 0 ) {
                continue;
            }

            if( errno != EEXIST ) {
                const std::error_code mkdir_error( errno, std::generic_category() );
                return edge::Make_Failure( Filesystem_Error(
                    Error_Code::BIND_FAILED, "failed to create IPC endpoint directory", directory, mkdir_error ) );
            }

            error.clear();
            if( !std::filesystem::is_directory( directory, error ) ) {
                if( error ) {
                    return edge::Make_Failure( Filesystem_Error(
                        Error_Code::BIND_FAILED, "failed to inspect IPC endpoint directory", directory, error ) );
                }
                return edge::Make_Failure( Error {
                    Error_Code::BIND_FAILED,
                    "IPC endpoint parent is not a directory: '" + directory.string() + "'",
                } );
            }
        }

        return {};
    }

}  // namespace edge::ipc::detail
