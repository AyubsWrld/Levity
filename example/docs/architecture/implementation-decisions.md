# Modular Edge ISR API — Implementation Decisions

This file records concrete Tier 1 implementation decisions and clarifications used by the coding harness. It does **not** replace or rewrite `modular_api_architecture.md`; that ATAM document remains the source of architectural rationale and quality attributes.

## 1. Development and production topologies are intentionally different

- **Development / integration:** the Product Monolith (or `dummy_monolith`), API Gateway, and Gimbal Controller run as three independent OS processes inside one sandboxed SDK development container. CLion may launch them with a Compound Run/Debug Configuration. Integration tests launch the executables directly.
- **Production:** the same responsibilities remain three independent containers orchestrated by Docker Compose.

The unified development container is a tooling convenience, not permission to introduce in-process coupling or service-to-service code dependencies.

## 2. Production UDS filesystem

All three production containers mount one **shared named Docker volume backed by Linux tmpfs** at:

```text
/tmp/edge-ipc
```

Do not model this as three separate service-level `tmpfs:` mounts; the containers must see the same filesystem namespace for ZeroMQ Unix-domain socket files.

## 3. Canonical Tier 1 IPC endpoints

Use these defaults:

```text
ipc:///tmp/edge-ipc/telemetry.ipc
ipc:///tmp/edge-ipc/gimbal-cmd.ipc
```

The shorter `/tmp/telemetry.ipc` and `/tmp/gimbal-cmd.ipc` examples in the ATAM flow are not the implementation defaults because they would fall outside the shared `/tmp/edge-ipc` mount. Endpoints remain configurable for integration tests and deployment.

## 4. Canonical Tier 1 process flow

For the Slew-to-Cue MVP, the detailed flow is authoritative:

```text
dummy_monolith PUB -> api_gateway SUB
external REST      -> api_gateway core
api_gateway REQ    -> gimbal_controller REP
gimbal core        -> STANAG adapter -> UDP
```

The Gimbal Controller does not separately subscribe to AI telemetry in Tier 1 unless a later requirement explicitly adds that behavior.

## 5. Protobuf is the domain contract

The shared generated Protobuf messages intentionally double as the internal Hexagonal domain contract. Do not add a mapping-only DTO/domain layer solely to isolate core code from Protobuf. A separate representation is justified only when it adds genuinely different invariants, behavior, or storage needs.

## 6. REQ/REP reliability semantics

REQ/REP supplies synchronous request/reply acknowledgement and sequencing; it is not treated as an exactly-once physical-command guarantee.

A timed-out slew command must not be blindly retried unless the operation is explicitly idempotent or the contract includes a command/request ID with receiver-side duplicate detection. Otherwise the timeout represents an ambiguous outcome.

## 7. IPC latency target

Sub-millisecond local IPC remains an architecture target. It is a **measured requirement**, not a property assumed from using UDS, ZeroMQ, or tmpfs. Development measurements can detect regressions; production claims require representative edge-hardware evidence.

## 8. Tier 2 lifecycle management remains undecided

`/start`, `/stop`, `/config`, monolith process restart, and GStreamer pipeline teardown/rebuild are outside Tier 1. A future task requiring lifecycle management is architectural work and must not silently choose between the ATAM's Docker-restart/"poison pill" and in-process standby-state-machine alternatives.

## 9. Project-wide implementation choices

- C++23.
- Conan 2 for third-party C++ dependency management.
- GoogleTest + GoogleMock only.
- Service tests in `services/<service>/test/`.
- Black-box multi-process integration tests in `test/integration/`.
- Integration tests launch binaries directly inside the development container, not through Docker Compose.

## 10. Copilot agent model policy

- High-level orchestration/planning/context architecture agents use **Claude Opus 5**.
- Focused implementation, build repair, testing, code review, and security review agents use **Claude Sonnet 5**.
- Agent files specify the model explicitly rather than inheriting a changing default.
