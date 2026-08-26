/**
 * @file limits.h
 * @brief Public resource limits enforced by the edge IPC transport wrappers.
 */
#pragma once

#include <cstddef>

namespace edge::ipc {

    /**
     * @brief Maximum serialized application payload accepted by an IPC wrapper.
     *
     * @details The limit is checked by the wrapper before sends and after receives. Native sockets
     * also set the corresponding libzmq maximum-message-size option so a non-conforming peer cannot
     * force allocation of an arbitrarily large frame before wrapper-level validation runs.
     */
    inline constexpr std::size_t MAX_SERIALIZED_MESSAGE_BYTES = 65536;

}  // namespace edge::ipc
