/**
 * @file endpoint.h
 * @brief Public endpoint configuration helpers and compatibility filesystem preparation API.
 */
#pragma once

#include <edge/ipc/endpoints.h>
#include <edge/ipc/error.h>
#include <edge/result.h>

#include <string>
#include <string_view>

namespace edge::ipc {

    /**
     * @brief Reads an IPC endpoint override from an environment variable.
     * @param variable Null-terminated environment-variable name.
     * @param fallback Endpoint returned when the variable is missing or empty.
     * @return Owned endpoint string.
     */
    [[nodiscard]] auto Endpoint_From_Environment( const char* variable, std::string_view fallback ) -> std::string;

    /** @brief Returns an owned copy of DEFAULT_TELEMETRY_ENDPOINT. */
    [[nodiscard]] inline auto Default_Telemetry_Endpoint() -> std::string {
        return std::string( DEFAULT_TELEMETRY_ENDPOINT );
    }

    /** @brief Returns an owned copy of DEFAULT_GIMBAL_COMMAND_ENDPOINT. */
    [[nodiscard]] inline auto Default_Gimbal_Command_Endpoint() -> std::string {
        return std::string( DEFAULT_GIMBAL_COMMAND_ENDPOINT );
    }

    /**
     * @brief Prepares the parent directory of an `ipc://` bind endpoint.
     * @param endpoint Endpoint whose filesystem parent should be validated/prepared.
     * @return Success or an IPC endpoint/filesystem error.
     *
     * @details This helper exists for compatibility with the original Tier-1 API. New production
     * code should call Publisher::Bind() or Replier::Bind(), which perform this operation internally.
     * Existing parent directories are validated but never chmod'ed; owner-only permissions are
     * applied only to directories created by the IPC layer itself. Existing socket files are never
     * blindly unlinked. When production services use different UIDs/groups, deployment should
     * pre-create the shared directory with the intended cross-service permissions.
     */
    [[nodiscard]] auto Ensure_Endpoint_Directory( std::string_view endpoint ) -> edge::Result<void, Error>;

}  // namespace edge::ipc
