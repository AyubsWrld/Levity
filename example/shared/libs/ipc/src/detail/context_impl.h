/**
 * @file context_impl.h
 * @brief Private native ZeroMQ implementation backing edge::ipc::Context.
 *
 * @details This header is compiled only inside libedge_ipc. Keeping `zmq.hpp` here prevents the
 * public IPC API from leaking its transport dependency to service adapters.
 */
#pragma once

#include <edge/ipc/context.h>
#include <zmq.hpp>

namespace edge::ipc {

    /** @brief Opaque native state owned by the public Context wrapper. */
    struct Context::Impl {
        /** Shared libzmq context from which all native sockets are created. */
        zmq::context_t context;
    };

}  // namespace edge::ipc
