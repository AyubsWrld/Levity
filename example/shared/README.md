# Shared code

`shared/` contains only code/contracts that are legitimately reusable across independent processes.
It is not a place for service-specific helpers.

```text
shared/
├── include/edge/
│   └── result.h      # generic Result/Status API (edge_common)
├── test/
│   └── result_test.cpp
├── schemas/          # Protobuf domain contracts (edge_schemas)
└── libs/
    ├── ipc/          # ZeroMQ UDS façade (edge_ipc)
    └── platform/     # small process/OS primitives (edge_platform)
```

## Include convention

Public C++ APIs keep a stable `edge/...` namespace in their include spelling:

```cpp
#include <edge/ipc/context.h>
#include <edge/platform/logging/logger.h>
```

That is why each public include root contains an `edge/<module>/` hierarchy. The extra physical
folders are intentional and avoid ambiguous global headers such as `<context.h>` or `<logger.h>`.
Implementation-only headers do not live in that public tree.

## Dependency intent

- `edge_common` exposes the generic `<edge/result.h>` expected-failure utility.
- `edge_schemas` is the shared Protobuf domain contract.
- `edge_ipc` hides ZeroMQ and filesystem/socket mechanics behind small C++20 RAII types.
- `edge_platform` contains policy-light utilities such as logging, environment parsing, POSIX FD
  ownership, and shutdown signal handling.
- service business logic must not be added here merely to make it shareable.

## Common result type

`include/edge/result.h` provides the header-only C++20 `edge::Result<T, E>` and `edge::Status<E>`
utilities used for expected operational failures. This keeps core and IPC APIs typed without depending
on C++20-incompatible standard-library facilities.
