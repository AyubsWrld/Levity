/**
 * @file context.cpp
 * @brief Implementation of context.
 *
 * @details Source path: `shared/libs/ipc/src/context.cpp`.
 */

#include "detail/context_impl.h"

#include <edge/ipc/context.h>

namespace edge::ipc {

    /** @copydoc Context::Context */
    Context::Context() : m_impl( std::make_unique<Impl>() ) {}
    /** @copydoc Context::~Context */
    Context::~Context() = default;

    /** @copydoc Context::Native_Impl */
    auto Context::Native_Impl() noexcept -> Impl& { return *m_impl; }

}  // namespace edge::ipc
