# dummy_monolith: intentionally flat bootstrap telemetry publisher (no hexagonal layers).
#
# See .github/bootstrap-mvp.md Phase 3 and docs/architecture/tier1-contracts.md for the contract
# this executable implements.

add_executable(dummy_monolith main.cpp)

target_link_libraries(dummy_monolith PRIVATE edge_schemas edge_ipc)

edge_configure_target(dummy_monolith)
