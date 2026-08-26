/**
 * @file ipc.h
 * @brief Convenience umbrella include for the complete public edge IPC API.
 *
 * @details Small tools/tests may include this header. Production adapters should normally include
 * only the role-specific headers they use to keep dependencies explicit.
 */
#pragma once

#include <edge/ipc/context.h>
#include <edge/ipc/endpoints.h>
#include <edge/ipc/error.h>
#include <edge/ipc/publisher.h>
#include <edge/ipc/replier.h>
#include <edge/ipc/requester.h>
#include <edge/ipc/subscriber.h>
