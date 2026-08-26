/**
 * @file error.h
 * @brief Transport-level error model shared by all public edge IPC socket wrappers.
 */
#pragma once

#include <cstdint>
#include <string>

namespace edge::ipc {

    /**
     * @brief Stable IPC failure categories exposed without leaking libzmq error types.
     */
    enum class Error_Code : std::uint8_t {
        /** A bounded send/receive/request deadline elapsed. */
        TIMEOUT,
        /** The peer or underlying transport is unavailable for the requested operation. */
        PEER_UNAVAILABLE,
        /** A bind operation failed. */
        BIND_FAILED,
        /** A connect operation failed. */
        CONNECT_FAILED,
        /** The transport could not send the complete logical message. */
        SEND_FAILED,
        /** The transport could not receive/parse the expected logical framing. */
        RECEIVE_FAILED,
        /** Payload exceeds MAX_SERIALIZED_MESSAGE_BYTES. */
        MESSAGE_TOO_LARGE,
        /** Endpoint syntax/path is invalid for the requested operation. */
        INVALID_ENDPOINT,
        /** Operation violates the REQ/REP or wrapper protocol state machine. */
        PROTOCOL_STATE_ERROR,
        /** Operation was interrupted/rejected because wrapper shutdown is in progress. */
        SHUTTING_DOWN,
    };

    /** @brief Public IPC failure containing a stable code plus human-readable diagnostic detail. */
    struct Error {
        /** Machine-readable category for adapter/application mapping. */
        Error_Code code;
        /** Human-readable diagnostic originating from validation or the native transport. */
        std::string detail;
    };

    /** @deprecated Compatibility alias; prefer Error_Code. */
    using Ipc_Error_Code = Error_Code;
    /** @deprecated Compatibility alias; prefer Error. */
    using Ipc_Error = Error;

}  // namespace edge::ipc
