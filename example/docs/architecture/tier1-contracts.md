# Tier 1 Derived Contract Decisions

This document records **implementation-level contract decisions derived for the Tier 1 Slew-to-Cue MVP**.

It does **not** replace `modular_api_architecture.md` (architectural rationale / ATAM) or
`implementation-decisions.md` (authoritative Tier 1 resolutions). It records the concrete wire/API
details that those documents leave unspecified but that Tier 1 code cannot be written without.

Every decision below is marked with its provenance:

- **DERIVED** — a mechanical consequence of an existing authoritative statement.
- **ENGINEERING** — a bounded engineering choice (limits, encodings, defaults) that is safe to make
  and is documented here instead of appearing as unexplained magic numbers.
- **NEEDS SIGN-OFF** — a genuine product/safety decision. A safe, fail-closed default is implemented
  so Tier 1 is testable, but the value must be confirmed by a domain owner before fielding.

---

## 0. Implementation status — semantic validation is deferred

Tier 1 currently implements the **transport and flow**, not semantic domain validation. By explicit
project direction, services assume the telemetry and command content they receive is well-formed.

Still enforced (transport safety, not domain semantics):

- `MAX_SERIALIZED_MESSAGE_BYTES` on every ZeroMQ frame, checked before allocating or parsing;
- Protobuf parse success/failure (a malformed payload is dropped, never treated as valid);
- `MAX_REST_BODY_BYTES` on the HTTP inbound adapter;
- unknown `target_uid` lookup failure in the gateway cue cache (this is routing, not validation).

Deferred (documented below so the values are settled when the work is scheduled): the coordinate range
checks in §2/§3, the duplicate-UID policy in §5, and the `MAX_UID_LENGTH`/`MAX_TARGETS` domain limits
in §7. Sections 2, 3, 5 and the domain rows of §7 are therefore **specification, not current
behaviour**. Nothing in the code should claim those checks exist until they are implemented.

---

## 1. `SlewCommand` carries the resolved target coordinates — DERIVED

`modular_api_architecture.md` §3 step 7 states the gimbal's outbound adapter "translates the Protobuf
fields into STANAG 4586 Message #200/#201 formatting and transmits the payload over UDP to physically
move the sensor". `implementation-decisions.md` §4 states the Gimbal Controller does **not** subscribe
to telemetry in Tier 1.

Therefore the gimbal has no other source of coordinates, and `SlewCommand` must carry them. The API
Gateway resolves `target_uid` against its cue cache and embeds the resolved `Target` in the command.

Consequence: an unknown UID is rejected by the gateway before any IPC occurs.

## 2. Altitude units and datum — NEEDS SIGN-OFF

`.github/copilot-instructions.md` §9 requires an explicitly documented altitude range and units. The
architecture documents do not state them.

Tier 1 implements:

| Property | Value |
| --- | --- |
| Field name | `altitude_m` |
| Units | metres |
| Datum | WGS-84 ellipsoidal height |
| Accepted range | `[-500.0, 20000.0]` |

Metres vs feet and ellipsoidal vs MSL/AGL materially change where a real sensor points. Confirm before
integrating real hardware.

## 3. Latitude / longitude — ENGINEERING

WGS-84 decimal degrees. `latitude_deg` in `[-90, 90]`, `longitude_deg` in `[-180, 180]`. Non-finite
values (NaN/±Inf) are rejected.

## 4. `CuePriorityList` ordering — ENGINEERING (fail-safe)

The name implies a priority ordering but no document defines one. Tier 1 therefore **preserves the
producer's order verbatim** and neither asserts nor relies on priority semantics. `GET /api/v1/cues`
returns targets in the order published. No component reorders, ranks, or truncates by priority.

## 5. Duplicate target UIDs — ENGINEERING (fail-closed)

A `CuePriorityList` containing two targets with the same `uid` is **rejected in full**; the cache is
not partially mutated. Rationale: last-wins de-duplication would silently pick one of two conflicting
positions for the same physical object, which is unsafe for a slew command. The rejection is counted
in `telemetry_messages_rejected`.

## 6. Cache freshness / stale-cache semantics — DELIBERATELY UNSPECIFIED

`.github/bootstrap-mvp.md` Phase 6 forbids inventing stale-cache or command-during-outage semantics.

Tier 1 implements only the simplest mechanical behaviour: the cache holds the last **valid** list that
was received; it is not cleared when the publisher disappears and has no TTL. This is **observed
behaviour, not a product guarantee**, and no test asserts it. Telemetry messages carry no timestamp or
sequence field, because adding one would imply freshness semantics that do not exist yet.

## 7. Bounded limits — ENGINEERING

All limits are named constants, never literals at the call site.

| Constant | Value | Enforced at |
| --- | --- | --- |
| `MAX_UID_LENGTH` | 64 bytes | gateway telemetry validation, gateway slew request, gimbal command validation |
| `MAX_TARGETS` | 64 per `CuePriorityList` | gateway telemetry ingest |
| `MAX_SERIALIZED_MESSAGE_BYTES` | 65536 (64 KiB) | checked before `ParseFromArray` on SUB, REQ and REP |
| `MAX_REST_BODY_BYTES` | 4096 (4 KiB) | HTTP inbound adapter (`413` on exceed) |
| `GIMBAL_REQUEST_TIMEOUT_MS` | 1000 | REQ send/recv deadline |
| `HTTP_COMMAND_WAIT_MS` | 1500 | bounded wait of an HTTP thread on the command worker |
| `COMMAND_QUEUE_CAPACITY` | 8 | gateway command queue (full ⇒ `503`) |
| `TELEMETRY_PUBLISH_INTERVAL_MS` | 200 (default, configurable) | `dummy_monolith` publish loop |

## 8. REST authentication / authorisation — NEEDS SIGN-OFF

`POST /api/v1/slew` moves physical hardware. Tier 1 implements **no authentication or authorisation**
and, to compensate, the HTTP server **binds to `127.0.0.1` by default**. Exposing the control plane on
a routable interface requires an explicit security decision (authN/authZ, transport protection,
rate limiting) that is out of Tier 1 scope. `EDGE_HTTP_HOST` can override the bind address; doing so in
a deployment is a security-relevant act.

## 9. REST contract — DERIVED (paths) / ENGINEERING (bodies)

Paths `GET /api/v1/cues` and `POST /api/v1/slew {"target_uid":"drone_01"}` come from the architecture
document. The remaining shapes are defined here.

```text
GET  /api/v1/cues   -> 200 {"targets":[{"uid":"drone_01","latitude_deg":34.05,
                                        "longitude_deg":-118.25,"altitude_m":150.0}]}
                       (empty cache is 200 with an empty array, never 404)

POST /api/v1/slew   <- {"target_uid":"drone_01"}
                    -> 200 {"status":"accepted","target_uid":"drone_01"}
                    -> 400 {"error":"invalid_request","detail":"..."}
                    -> 404 {"error":"unknown_target","target_uid":"..."}
                    -> 409 {"error":"gimbal_rejected","detail":"..."}
                    -> 413 {"error":"body_too_large"}
                    -> 503 {"error":"gimbal_unavailable","detail":"..."}
                    -> 504 {"error":"gimbal_timeout","outcome":"unknown"}

GET  /api/v1/ready  -> 200 {"status":"ready"}
GET  /api/v1/status -> 200 {"telemetry_messages_ingested":42,
                            "telemetry_messages_rejected":0,
                            "cached_target_count":3}
```

`GET /api/v1/ready` reports only that the HTTP listener and adapters are constructed. It must **not**
require telemetry, because the integration harness polls readiness before starting `dummy_monolith`.

`504` is an **ambiguous outcome**: the command may or may not have been executed. Per
`implementation-decisions.md` §6 it is never retried automatically, and the response body says so.

## 10. Dummy STANAG 4586 #200/#201 representation — ENGINEERING

> **This is a project-local dummy representation. It is deliberately *not* wire-compatible with the
> controlled NATO STANAG 4586 specification**, which is not available to this repository. It exists so
> the Tier 1 outbound hardware adapter and its tests have a fixed, verifiable byte contract. Replacing
> it with the real specification is an adapter-local change by design.

One accepted slew emits **two UDP datagrams in order: #200 then #201**. All integers and doubles are
little-endian; structures are packed with no padding; doubles are IEEE-754 binary64.

Common 12-byte header:

| Offset | Size | Field | Value |
| --- | --- | --- | --- |
| 0 | 4 | `magic` | ASCII `"EDGE"` |
| 4 | 2 | `message_id` | `200` / `201` |
| 6 | 2 | `message_length` | total datagram length including header |
| 8 | 4 | `sequence` | monotonic per process, starts at 1, shared by both datagrams of one slew |

Message #200 — Steering Command (body 24 bytes, total 36):

| Offset | Size | Field |
| --- | --- | --- |
| 12 | 8 | `latitude_deg` (binary64) |
| 20 | 8 | `longitude_deg` (binary64) |
| 28 | 8 | `altitude_m` (binary64) |

Message #201 — Steering Mode (body 8 bytes, total 20):

| Offset | Size | Field |
| --- | --- | --- |
| 12 | 4 | `mode` (`1` = slew-to-coordinate) |
| 16 | 4 | `reserved` (`0`) |

Reference vector — `lat = 34.05`, `lon = -118.25`, `alt = 150.0`, `sequence = 1`:

```text
#200 (36 bytes)
  45 44 47 45  C8 00  24 00  01 00 00 00
  66 66 66 66 66 06 41 40
  00 00 00 00 00 90 5D C0
  00 00 00 00 00 C0 62 40

#201 (20 bytes)
  45 44 47 45  C9 00  14 00  01 00 00 00
  01 00 00 00  00 00 00 00
```

`target_uid` is intentionally absent from the wire payload; correlation stays in logs and in the
`SlewCommandReply`.

## 11. Framing — ENGINEERING

- Telemetry PUB/SUB: two frames — `["cues"][serialized CuePriorityList]`. The subscriber sets the
  `"cues"` filter explicitly.
- Command REQ/REP: one frame each — `[serialized SlewCommand]` / `[serialized SlewCommandReply]`.
- Frame size is validated against `MAX_SERIALIZED_MESSAGE_BYTES` **before** parsing.

## 12. Configuration surface — ENGINEERING

| Variable | Default | Used by |
| --- | --- | --- |
| `EDGE_TELEMETRY_ENDPOINT` | `ipc:///tmp/edge-ipc/telemetry.ipc` | `dummy_monolith` (bind), `api_gateway` (connect) |
| `EDGE_GIMBAL_COMMAND_ENDPOINT` | `ipc:///tmp/edge-ipc/gimbal-cmd.ipc` | `api_gateway` (connect), `gimbal_controller` (bind) |
| `EDGE_HTTP_HOST` | `127.0.0.1` | `api_gateway` |
| `EDGE_HTTP_PORT` | `8080` | `api_gateway` |
| `EDGE_STANAG_UDP_HOST` | `127.0.0.1` | `gimbal_controller` |
| `EDGE_STANAG_UDP_PORT` | `14550` | `gimbal_controller` |
| `EDGE_TELEMETRY_INTERVAL_MS` | `200` | `dummy_monolith` |

No configuration source requires WAN or cloud connectivity.

## 13. Out of scope for Tier 1

`/start`, `/stop`, `/config`, monolith restart strategy and GStreamer pipeline teardown/rebuild remain
deferred Tier 2 architectural decisions (`implementation-decisions.md` §8). Nothing in this document
introduces them.
