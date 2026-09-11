// Deterministic ELF fixture standing in for a real LGPL shared library, so
// dynamic-link tests don't depend on whatever happens to be installed on the
// host (FFmpeg, OpenSSL, ...).

extern "C" auto lgpl_fixture_value() -> int { return 42; }
