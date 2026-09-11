# Levity real Conan/Trivy LGPL integration fixture

* **Upstream repository:** [conan-io/template](https://github.com/conan-io/template)
* **Upstream commit:** `4e133a15da18036aac3666dfac89ccdbd5719574` ("Fix GH CI badge (#2)")
* **Purpose:** real Conan/Trivy LGPL integration fixture for Levity's `IsDynamicallyLinked` predicate

This directory is a snapshot of the upstream template above (a small, real Conan 2 +
CMake C++ application with a real `zlib` dependency), extended with a real
LGPL-2.1-or-later Conan dependency (`libiconv/1.18`) so it can be used as an
end-to-end integration fixture for Levity.

## Why this exists

Levity's unit tests (`../../tests/`) already prove the SONAME/`DT_NEEDED`
intersection *algorithm* is correct, using synthetic ELF fixtures and a fake
package resolver. They do not prove that Levity's real production path --
`ConanPackageResolver` reading the real Conan 2 local cache -- actually works
against a real package manager. This fixture closes that gap.

**This integration verifies Levity's ability to map a real SPDX Conan package
to its real shared-library artifact and determine whether the target ELF
contains that library's SONAME in `DT_NEEDED`.**

**Passing `IsDynamicallyLinked` does not establish complete LGPL compliance;
it only verifies the dynamic-link integration condition represented by this
predicate.** Notices, license text, source availability, relinking rights,
modifications, and written offers are separate concerns, not covered here.

## Why libiconv

`libiconv/1.18` is a real, actively-maintained Conan Center package licensed
`LGPL-2.1-or-later` that can be built `shared=True` on Linux, making it a
realistic stand-in for "the LGPL dependency" a real product would ship. The
existing `zlib` dependency (permissive `Zlib` license) is kept alongside it so
the generated SBOM contains more than a deliberately-isolated LGPL-only
package, proving Levity selects the LGPL suite only for the package that
actually needs it.

## Why both a linked and an unlinked build exist

`CMakeLists.txt` exposes `ENABLE_LGPL_LINK` (default `ON`). In both settings,
`libiconv/1.18` remains a real Conan requirement in `conanfile.py`, so it
always appears in `conan.lock` and in the Trivy-generated SBOM -- the SBOM
does not change between the two cases. Only the application's own linkage
changes:

* `ENABLE_LGPL_LINK=ON` -- `src/lgpl_link.cpp` is compiled in, calls the real
  `iconv_open`/`iconv_close` API, and the executable links `Iconv::Iconv`.
  libiconv's SONAME ends up in `DT_NEEDED`.
* `ENABLE_LGPL_LINK=OFF` -- the application neither calls libiconv nor links
  its CMake target. libiconv's SONAME is absent from `DT_NEEDED`.

This is the real negative condition `IsDynamicallyLinked` is supposed to
catch: an LGPL package the SBOM says the product depends on, that the actual
binary does not dynamically link.

## Required tools

`conan` (2.x), `trivy`, `cmake`, `readelf` (binutils), a C++23 compiler, and
`ninja` or `make`. See `run-integration.sh`, which checks all of these before
doing anything else.

## How to run the integration

From this directory:

```bash
./run-integration.sh
```

This resolves the real Conan graph, installs it into the local Conan cache,
generates a real Trivy SPDX SBOM, builds both the linked and unlinked
variants, inspects their real ELF `DT_NEEDED` with `readelf`, runs Levity's
actual `rbuff` CLI against both (no fake resolver, no edited SBOM), and
finally re-runs Levity's own unit/integration test suite as a regression
check.

## Expected results

* **Positive** (`build/integration-linked/myapp`): `DT_NEEDED` contains
  libiconv's real SONAME (currently `libiconv.so.2`); Levity reports
  `LGPL-2.1-or-later / IsDynamicallyLinked = PASS`.
* **Negative** (`build/integration-unlinked/myapp`): `DT_NEEDED` does not
  contain it; Levity reports `LGPL-2.1-or-later / IsDynamicallyLinked = FAIL`
  with a diagnostic distinguishing "resolved but not linked" from "unable to
  resolve" -- specifically **not** `Unresolved`.

## What this proves

1. Conan resolves a real `libiconv/1.18` binary package (`shared=True`).
2. Trivy discovers it from the real `conan.lock` + local Conan cache and
   emits its LGPL license and a Conan PURL in the SPDX `externalRefs`.
3. Levity parses that `ExternalRef`/PURL and dispatches to
   `ConanPackageResolver` -- not a fake.
4. `ConanPackageResolver` finds the real cached package and its real `.so`.
5. Levity extracts the real ELF SONAME and the target's real `DT_NEEDED`.
6. Exact SONAME intersection correctly yields `Linked` and `NotLinked` in the
   two configurations, and the predicate reports `PASS`/`FAIL` accordingly.

## What this does NOT prove

All LGPL obligations beyond dynamic linkage (notices, license text, source
offers, relinkability, modifications), static-link compliance, GPL/MPL rules,
`dlopen`/plugin loading, Windows PE or macOS Mach-O dependency inspection,
`DebianPackageResolver` (a separate resolver, not exercised by this fixture),
or every possible Conan recipe shape. This is one real path: Trivy SPDX +
Conan 2 + a shared LGPL library on Linux ELF.

---

The remainder of this README is the original upstream template's
documentation for building/running `myapp` directly (without the Levity
integration wrapper).

# C++ Project Template using GTest and Conan

[![CI](https://github.com/conan-io/template/actions/workflows/ci.yaml/badge.svg)](https://github.com/conan-io/template/actions/workflows/ci.yaml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A modern C++ project demonstrating cross-platform builds with Conan package manager, featuring zlib compression utilities and comprehensive testing with Google Test.

## 🚀 Features

- **Modern C++17** codebase
- **Cross-platform CI/CD** with GitHub Actions
- **Multi-architecture builds** (x86_64 and armv8)
- **Conan package management** for dependency management
- **Google Test integration** for unit testing

## 📋 Prerequisites

- **Conan** 2.x
- **A C++17 compatible compiler**:
  - GCC 7+
  - Clang 5+
  - MSVC 2017+
- **Python** 3.7+ (for Conan)

## 🛠️ Building the Project

### Install Dependencies

```bash
# Install Conan if you haven't already
pip install conan

# Detect your Conan profile
conan profile detect --force
```

### Build

```bash
# Build the project (installs dependencies and compiles)
conan build . --build=missing
```

### Run the Application

```bash
# Linux/macOS
./build/Release/myapp

# Windows
.\build\Release\myapp.exe
```

### Run Tests

If the tests were not skipped during the build step, you can run them with:

```bash
ctest --preset conan-release
```
