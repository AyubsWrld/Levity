# edge_ipc

Small RAII wrappers around the Tier-1 ZeroMQ Unix-domain IPC patterns.

## Public vs private headers

Consumer code only sees headers under:

```text
include/edge/ipc/
```

ZeroMQ and implementation helpers live under:

```text
src/detail/
```

`src/` is a **private** CMake include directory. Adapters therefore cannot accidentally include the
ZeroMQ implementation layer.

The physical `include/edge/ipc/...` hierarchy is intentional: C/C++ include spelling follows the
filesystem below an include root. Keeping this hierarchy gives consumers the unambiguous imports we
want:

```cpp
#include <edge/ipc/context.h>
#include <edge/ipc/requester.h>
```

Flattening the files to `include/context.h` would either force ambiguous `<context.h>` imports or
require generated/symlink include mappings. The extra physical directories are preferable to making
consumer includes less clear.

## Typical use

Binder:

```cpp
#include <edge/ipc/context.h>
#include <edge/ipc/publisher.h>

edge::ipc::Context context;
auto publisher = edge::ipc::Publisher::Bind(
    context,
    "ipc:///tmp/edge-ipc/telemetry.ipc");
if (!publisher) {
    // publisher.error()
}
```

Connector:

```cpp
#include <edge/ipc/context.h>
#include <edge/ipc/subscriber.h>

edge::ipc::Context context;
auto subscriber = edge::ipc::Subscriber::Connect(
    context,
    "ipc:///tmp/edge-ipc/telemetry.ipc",
    "cues");
```

Keep the `Context` alive longer than every socket created from it. A socket object must be created,
used, and destroyed on its owning thread.

## Endpoint ownership

`Publisher::Bind()` and `Replier::Bind()` validate `ipc://` endpoints and create missing parent
directories automatically. Newly created directories are restricted to owner access (`0700`).
Existing parent directories are never chmod'd, and the library never unlinks an existing socket file
that it did not own.

Callers therefore do not perform bind-directory preparation themselves.

Use `Bind()` for local PUB/REP endpoints and `Connect()` for SUB/REQ endpoints. The original
`Create()` spellings and `endpoint.h` helpers remain only as compatibility shims for older Tier-1
callers that were not part of this refactor.

## Errors and REQ/REP semantics

Public operations return `edge::Result<..., edge::ipc::Error>` for normal IPC failures; consumer
code does not depend on `zmq::error_t`.

REQ/REP acknowledgement is not an exactly-once physical-command guarantee. A timeout after send is
an ambiguous outcome and is never automatically retried. The requester recreates its REQ socket only
so a **later, distinct** request can start from a valid state.
