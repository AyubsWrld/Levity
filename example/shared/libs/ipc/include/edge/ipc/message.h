/**
 * @file message.h
 * @brief Logical topic/payload message returned by the PUB/SUB wrapper.
 */
#pragma once

#include <string>

namespace edge::ipc {

    /**
     * @brief Owned PUB/SUB message reconstructed from the transport framing.
     *
     * @details Both strings own their storage, so the Message may safely outlive any native ZeroMQ
     * message object or Receive() call.
     */
    struct Message {
        /** Topic frame used for subscription filtering/routing. */
        std::string topic;
        /** Serialized application payload frame. */
        std::string payload;
    };

}  // namespace edge::ipc
