# Modular Edge ISR API Platform — Tier 1 Slew-to-Cue MVP

A C++23 monorepo implementing the Tier 1 **Slew-to-Cue** control plane: an AI/telemetry producer, an
operator-facing REST gateway, and a gimbal/sensor controller, running as **three independent processes**
that communicate over ZeroMQ Unix-domain IPC.

The architectural driver is **fault isolation**: the volatile data plane (the product monolith) can crash
without severing the operator's command-and-control link to the payload.

## Tier 1 flow

```text
dummy_monolith
    │  ZeroMQ PUB  ->  CuePriorityList          ipc:///tmp/edge-ipc/telemetry.ipc
    ▼
api_gateway
    │  GET  /api/v1/cues        current target list
    │  POST /api/v1/slew        {"target_uid":"drone_01"}
    │  GET  /api/v1/ready       readiness (independent of telemetry)
    │  GET  /api/v1/status      ingest counters
    │
    │  ZeroMQ REQ  ->  SlewCommand              ipc:///tmp/edge-ipc/gimbal-cmd.ipc
    ▼
gimbal_controller
    │  dummy STANAG 4586 Message #200 / #201
    ▼
UDP hardware endpoint
```

`gimbal_controller` deliberately does **not** subscribe to telemetry in Tier 1; the gateway resolves the
UID against its cue cache and embeds the coordinates in the `SlewCommand`.

## Documentation

| Document | Role |
| --- | --- |
| `docs/architecture/modular_api_architecture.md` | ATAM architectural rationale and quality attributes |
| `docs/architecture/implementation-decisions.md` | Authoritative Tier 1 resolutions |
| `docs/architecture/tier1-contracts.md` | Derived wire/API contracts: proto fields, REST shapes, limits, dummy STANAG byte layout, env vars |
| `.github/copilot-instructions.md` | Permanent engineering rules and Definition of Done |
| `.github/bootstrap-mvp.md` | Bootstrap phases for this MVP |

**Read `docs/architecture/tier1-contracts.md` §0 first.** Semantic domain validation (coordinate ranges,
UID length, duplicate-UID policy) is currently **deferred by project direction**; services assume
well-formed content. Transport-level limits, protobuf parse failures and HTTP body limits *are* enforced.

## Repository layout

```text
├── CMakeLists.txt              root build: options, find_package, architecture checks
├── conanfile.py                Conan 2 recipe, exact pinned versions
├── cmake/
│   ├── edge_target_helpers.cmake    C++23 + warnings + sanitizers + test registration
│   └── architecture_checks.cmake    configure-time dependency-direction enforcement
├── app/dummy_monolith/         intentionally flat PUB telemetry producer
├── services/
│   ├── api_gateway/            core/ + adapters/ + test/   (REST, SUB, REQ)
│   └── gimbal_controller/      core/ + adapters/ + test/   (REP, dummy STANAG UDP)
├── shared/
│   ├── schemas/                telemetry.proto, commands.proto  -> edge_schemas
│   └── libs/ipc/               RAII ZeroMQ wrappers            -> edge_ipc
├── test/integration/           black-box multi-process tests
├── docker-compose.yml          production three-container topology
└── deploy/Dockerfile.runtime   production runtime image (one service per image)
```

### Target graph

```text
edge_schemas ─┬─> api_gateway_core ──> api_gateway_adapters ──> api_gateway
              └─> gimbal_core      ──> gimbal_adapters      ──> gimbal_controller
edge_ipc ─────────────────────────────^ (PRIVATE to adapters, and to dummy_monolith)
```

`cmake/architecture_checks.cmake` fails the configure step if a core target ever links HTTP/ZeroMQ/JSON
or an adapter target, if either service links the other, or if the integration suite links service code.
Protobuf is the intentional shared domain contract, so `edge_schemas` on a core target is the one
sanctioned exception — there is no mapping-only DTO layer.

## Build

Requires the repository's Ubuntu 24.04 development container (GCC 13+, CMake 3.24+, Ninja, Conan 2).

```bash
cmake -S . -B cmake-build-debug -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DSKIP_CLANG_TIDY=ON

cmake --build cmake-build-debug
```

`conan install` runs automatically at configure time (see the root `CMakeLists.txt`), so there is no
separate dependency-resolution step. The first configure builds ZeroMQ, Protobuf and GoogleTest from
source (roughly ten minutes); it is cached afterwards.

### CMake options

| Option | Default | Effect |
| --- | --- | --- |
| `SKIP_CLANG_TIDY` | `ON` | clang-tidy is currently skipped for fast iteration |
| `EDGE_WARNINGS_AS_ERRORS` | `OFF` | add `-Werror` to project targets |
| `EDGE_BUILD_TESTS` | `ON` | build unit and integration tests |
| `EDGE_BUILD_BENCHMARKS` | `ON` | build `edge_ipc_bench` |
| `EDGE_ENABLE_ASAN` / `EDGE_ENABLE_UBSAN` / `EDGE_ENABLE_TSAN` | `OFF` | sanitizers on project targets (ASan and TSan are mutually exclusive) |

## Test

```bash
ctest --test-dir cmake-build-debug --output-on-failure          # everything
ctest --test-dir cmake-build-debug -L unit --output-on-failure  # unit/component only
ctest --test-dir cmake-build-debug -L integration --output-on-failure
ctest --test-dir cmake-build-debug -R Fault_Isolation --output-on-failure
```

Integration tests launch the three built executables **directly as independent OS processes inside the
development container**. Docker Compose is never used to run tests. Each fixture allocates a unique
temporary IPC directory, an ephemeral HTTP port and an ephemeral UDP port, polls observable readiness
conditions with hard deadlines (never fixed sleeps), and unconditionally reaps every child process.

The mandatory fault-isolation acceptance case kills `dummy_monolith`, proves `api_gateway` stays alive and
HTTP-responsive and `gimbal_controller` stays alive, restarts only the monolith, and proves telemetry
ingestion recovers within a bounded deadline.

## Running the three processes manually (CLion compound configuration)

```bash
mkdir -p /tmp/edge-ipc
./cmake-build-debug/bin/gimbal_controller &
./cmake-build-debug/bin/api_gateway &
./cmake-build-debug/bin/dummy_monolith &

curl -s http://127.0.0.1:8080/api/v1/cues
curl -s -X POST http://127.0.0.1:8080/api/v1/slew \
     -H 'Content-Type: application/json' -d '{"target_uid":"drone_01"}'
```

All three executables land in `cmake-build-debug/bin/`, which is exactly CLion's default build directory
name, so they can be launched and debugged directly from CLion as a Compound Run/Debug Configuration with
no extra setup. Docker Compose is not a prerequisite for development.

### Configuration

Every endpoint is environment-overridable so tests and deployments can isolate themselves; see
`docs/architecture/tier1-contracts.md` §12 for the full list and defaults. No configuration source
requires WAN or cloud connectivity.

## IPC latency benchmark

```bash
./cmake-build-debug/bin/edge_ipc_bench <iterations> <payload_bytes>
```

Reports p50/p95/p99 for REQ/REP round trip and PUB/SUB one-way. **Development-container measurements only.**
Sub-millisecond local IPC is a *measured* architecture target, not a property assumed from using ZeroMQ,
Unix-domain sockets or tmpfs; a production claim requires representative edge-hardware measurement.

## Production deployment

`docker-compose.yml` defines the three-container production topology. All three containers mount **one
shared named Docker volume backed by Linux tmpfs** at `/tmp/edge-ipc` — not three separate `tmpfs:` mounts —
so they share a filesystem namespace for the Unix-domain socket files. The Docker socket is never mounted;
containers run unprivileged with `cap_drop: ALL`, `no-new-privileges` and a read-only root filesystem; the
gateway's published port is bound to the host loopback interface because Tier 1 has no authentication.

Deployment is separate from development: Compose is **not** the agentic development or integration-test
orchestration mechanism.

## Out of scope (deferred Tier 2)

`/start`, `/stop`, `/config`, monolith restart strategy and GStreamer pipeline teardown/rebuild are
deliberately deferred architectural decisions (`implementation-decisions.md` §8).
