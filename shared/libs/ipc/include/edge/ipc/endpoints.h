/**
 * @file endpoints.h
 * @brief Canonical default IPC endpoint constants for the Tier-1 architecture.
 */
#pragma once

#include <string_view>

namespace edge::ipc {

    /** Default ZeroMQ PUB/SUB endpoint carrying CuePriorityList telemetry. */
    inline constexpr std::string_view DEFAULT_TELEMETRY_ENDPOINT = "ipc:///tmp/edge-ipc/telemetry.ipc";
    /** Default ZeroMQ REQ/REP endpoint carrying gimbal SlewCommand requests/replies. */
    inline constexpr std::string_view DEFAULT_GIMBAL_COMMAND_ENDPOINT = "ipc:///tmp/edge-ipc/gimbal-cmd.ipc";

}  // namespace edge::ipc
