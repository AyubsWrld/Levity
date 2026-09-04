/**
 * @file socket.h
 * @brief Private libzmq socket configuration and error-mapping utilities.
 */
#pragma once

#include <edge/ipc/error.h>
#include <edge/ipc/limits.h>
#include <zmq.hpp>

#include <chrono>

namespace edge::ipc::detail {

    /** Default native socket timeout used when a wrapper has not supplied a per-operation override. */
    inline constexpr std::chrono::milliseconds DEFAULT_SOCKET_TIMEOUT { 5000 };

    /**
     * @brief Applies common bounded/lifecycle-safe options to a newly created native socket.
     * @param socket Native socket owned by the current IPC wrapper/thread.
     */
    void Configure_New_Socket( zmq::socket_t& socket );

    /**
     * @brief Normalizes an application timeout to the finite range accepted by libzmq.
     * @param timeout Requested timeout. Non-positive values become zero (non-blocking).
     * @return Timeout clamped to `[0, INT_MAX]` milliseconds.
     *
     * @details libzmq represents these socket timeouts as signed `int` milliseconds and uses
     * negative values for infinite waits. The IPC layer deliberately never turns an application
     * timeout into an accidental infinite wait or permits integer overflow during conversion.
     */
    [[nodiscard]] auto Normalize_Timeout( std::chrono::milliseconds timeout ) noexcept -> std::chrono::milliseconds;

    /**
     * @brief Applies a finite native send timeout.
     * @param socket Native socket to configure.
     * @param timeout Requested timeout; normalized by Normalize_Timeout().
     */
    void Set_Send_Timeout( zmq::socket_t& socket, std::chrono::milliseconds timeout );

    /**
     * @brief Applies a finite native receive timeout.
     * @param socket Native socket to configure.
     * @param timeout Requested timeout; normalized by Normalize_Timeout().
     */
    void Set_Receive_Timeout( zmq::socket_t& socket, std::chrono::milliseconds timeout );

    /** @brief Converts a libzmq bind exception into the stable public Error model. */
    [[nodiscard]] auto Map_Bind_Error( const zmq::error_t& error ) -> Error;
    /** @brief Converts a libzmq connect exception into the stable public Error model. */
    [[nodiscard]] auto Map_Connect_Error( const zmq::error_t& error ) -> Error;

}  // namespace edge::ipc::detail
