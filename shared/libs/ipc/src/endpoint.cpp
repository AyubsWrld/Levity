/**
 * @file endpoint.cpp
 * @brief Implementation of endpoint.
 *
 * @details Source path: `shared/libs/ipc/src/endpoint.cpp`.
 */

#include "detail/endpoint.h"

#include <edge/ipc/endpoint.h>

#include <cstdlib>

namespace edge::ipc {

    /** @copydoc Endpoint_From_Environment() */
    auto Endpoint_From_Environment( const char* variable, std::string_view fallback ) -> std::string {
        if( variable != nullptr ) {
            if( const char* value = std::getenv( variable );  // NOLINT(concurrency-mt-unsafe)
                value != nullptr && *value != '\0' ) {
                return value;
            }
        }
        return std::string( fallback );
    }

    /** @copydoc Ensure_Endpoint_Directory() */
    auto Ensure_Endpoint_Directory( std::string_view endpoint ) -> edge::Result<void, Error> {
        return detail::Prepare_Bind_Endpoint( endpoint );
    }

}  // namespace edge::ipc
