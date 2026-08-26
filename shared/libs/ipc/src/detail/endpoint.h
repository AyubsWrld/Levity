/**
 * @file endpoint.h
 * @brief Private endpoint validation and bind-side filesystem preparation helpers.
 */
#pragma once

#include <edge/ipc/error.h>
#include <edge/result.h>

#include <string_view>

namespace edge::ipc::detail {

    /**
     * @brief Validates endpoint syntax supported by the IPC wrappers.
     * @param endpoint Candidate endpoint string.
     * @return Success or INVALID_ENDPOINT with diagnostic detail.
     */
    [[nodiscard]] auto Validate_Endpoint( std::string_view endpoint ) -> edge::Result<void, Error>;

    /**
     * @brief Validates a bind endpoint and creates a missing `ipc://` parent directory when needed.
     * @param endpoint Bind endpoint.
     * @return Success or endpoint/filesystem Error.
     *
     * @details Newly created directories receive owner-only permissions. Existing directories are
     * not chmod'ed, and existing socket files are not unlinked automatically.
     */
    [[nodiscard]] auto Prepare_Bind_Endpoint( std::string_view endpoint ) -> edge::Result<void, Error>;

}  // namespace edge::ipc::detail
