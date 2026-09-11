#!/usr/bin/env bash
#
# Real Conan + Trivy + Levity LGPL dynamic-link integration.
#
# Proves the production chain end to end:
#   real Conan dependency (libiconv/1.18, LGPL-2.1-or-later)
#     -> real conan.lock
#     -> real Trivy SPDX SBOM
#     -> real SPDX PURL
#     -> Levity's ConanPackageResolver (no fakes)
#     -> real .so artifact from the Conan cache
#     -> real ELF SONAME
#     -> real target ELF DT_NEEDED
#     -> LGPL IsDynamicallyLinked predicate
#     -> PASS (linked) / FAIL (unlinked)
#
# See README.md for what this does and does not prove.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LEVITY_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${SCRIPT_DIR}"

export CMAKE_POLICY_VERSION_MINIMUM="${CMAKE_POLICY_VERSION_MINIMUM:-3.5}"

log_stage() { printf '\n=== %s ===\n' "$1"; }
fail() { printf 'INTEGRATION FAILED: %s\n' "$1" >&2; exit 1; }

# ---------------------------------------------------------------------------
log_stage "Check prerequisites"
# ---------------------------------------------------------------------------
for tool in conan trivy readelf cmake c++; do
    command -v "${tool}" >/dev/null 2>&1 || fail "required tool not found on PATH: ${tool}"
done
if command -v ninja >/dev/null 2>&1; then
    GENERATOR=Ninja
elif command -v make >/dev/null 2>&1; then
    GENERATOR="Unix Makefiles"
else
    fail "neither ninja nor make found on PATH"
fi
echo "conan:   $(conan --version)"
echo "trivy:   $(trivy --version | head -1)"
echo "readelf: $(readelf --version | head -1)"
echo "cmake:   $(cmake --version | head -1)"
echo "c++:     $(c++ --version | head -1)"
echo "generator: ${GENERATOR}"

# ---------------------------------------------------------------------------
log_stage "Ensure Levity (rbuff) is built"
# ---------------------------------------------------------------------------
RBUFF="${LEVITY_ROOT}/build/rbuff"
if [ ! -x "${RBUFF}" ]; then
    echo "rbuff not found at ${RBUFF}; building Levity first"
    (cd "${LEVITY_ROOT}" && cmake --build build)
fi
[ -x "${RBUFF}" ] || fail "rbuff still not found/executable at ${RBUFF} after build"
echo "rbuff: ${RBUFF}"

# ---------------------------------------------------------------------------
log_stage "Resolve the real Conan dependency graph (conan.lock)"
# ---------------------------------------------------------------------------
if [ ! -f conan.lock ]; then
    echo "conan.lock missing; creating it"
    conan lock create . -s build_type=Release --lockfile-out=conan.lock
fi
grep -q '"libiconv/1.18' conan.lock || fail "conan.lock does not contain libiconv/1.18"
echo "conan.lock contains libiconv/1.18"

# ---------------------------------------------------------------------------
log_stage "Install the real dependency graph into the local Conan cache"
# ---------------------------------------------------------------------------
# Packages must exist in the cache before Trivy runs -- Trivy's Conan
# analyzer reads recipe license metadata from the cache, not from conan.lock.
conan install . --lockfile=conan.lock -s build_type=Release --build=missing

# ---------------------------------------------------------------------------
log_stage "Verify the real libiconv package in the Conan cache"
# ---------------------------------------------------------------------------
LIBICONV_LIST_JSON="$(conan list 'libiconv/1.18:*' --format=json 2>/dev/null)"
read -r LIBICONV_REF LIBICONV_PACKAGE_ID LIBICONV_SHARED <<< "$(python3 - "${LIBICONV_LIST_JSON}" <<'PY'
import json, sys
data = json.loads(sys.argv[1])
cache = data["Local Cache"]
ref = next(k for k in cache if k.startswith("libiconv/1.18"))
revisions = cache[ref]["revisions"]
rrev = next(iter(revisions))
packages = revisions[rrev]["packages"]
pkg_id = next(iter(packages))
shared = packages[pkg_id]["info"]["options"].get("shared", "False")
print(ref, pkg_id, shared)
PY
)"
[ -n "${LIBICONV_PACKAGE_ID:-}" ] || fail "could not find a cached libiconv/1.18 binary package"
[ "${LIBICONV_SHARED}" = "True" ] || fail "cached libiconv/1.18 package is not shared=True (got: ${LIBICONV_SHARED})"
LIBICONV_PKG_PATH="$(conan cache path "libiconv/1.18:${LIBICONV_PACKAGE_ID}" 2>/dev/null)"
[ -d "${LIBICONV_PKG_PATH}" ] || fail "conan cache path did not resolve a real folder for libiconv/1.18:${LIBICONV_PACKAGE_ID}"
[ -e "${LIBICONV_PKG_PATH}/lib/libiconv.so" ] || fail "no libiconv.so found under ${LIBICONV_PKG_PATH}/lib"
LIBICONV_SO="$(realpath "${LIBICONV_PKG_PATH}/lib/libiconv.so")"
[ -f "${LIBICONV_SO}" ] || fail "libiconv.so symlink under ${LIBICONV_PKG_PATH}/lib does not resolve to a real file"
LIBICONV_SONAME="$(readelf -d "${LIBICONV_SO}" | grep SONAME | grep -oP '(?<=\[)[^]]+(?=\])' | head -1)"
[ -n "${LIBICONV_SONAME}" ] || fail "could not extract SONAME from ${LIBICONV_SO}"
echo "libiconv reference:    ${LIBICONV_REF}"
echo "libiconv package ID:   ${LIBICONV_PACKAGE_ID}"
echo "libiconv package path: ${LIBICONV_PKG_PATH}"
echo "libiconv shared:       ${LIBICONV_SHARED}"
echo "libiconv artifact:     ${LIBICONV_SO}"
echo "libiconv SONAME:       ${LIBICONV_SONAME}"

# ---------------------------------------------------------------------------
log_stage "Generate the real Trivy SPDX SBOM"
# ---------------------------------------------------------------------------
mkdir -p artifacts
trivy fs --scanners license,vuln --format spdx-json --output artifacts/sbom.spdx.json .
SBOM="${SCRIPT_DIR}/artifacts/sbom.spdx.json"

SPDX_VERSION="$(python3 -c "import json; print(json.load(open('${SBOM}'))['spdxVersion'])")"
echo "SBOM spdxVersion: ${SPDX_VERSION}"

read -r SBOM_NAME SBOM_VERSION SBOM_LICENSE SBOM_PURL <<< "$(python3 - "${SBOM}" <<'PY'
import json, sys
d = json.load(open(sys.argv[1]))
for p in d["packages"]:
    if p.get("name") == "libiconv":
        purl = next((r["referenceLocator"] for r in p.get("externalRefs", [])
                     if r.get("referenceType") == "purl"), "NONE")
        print(p["name"], p.get("versionInfo", "NONE"), p.get("licenseConcluded", "NONE"), purl)
        break
else:
    print("MISSING", "MISSING", "MISSING", "MISSING")
PY
)"
[ "${SBOM_NAME}" = "libiconv" ] || fail "Trivy SBOM does not contain a libiconv package"
[ "${SBOM_PURL}" != "NONE" ] || fail "libiconv SBOM package has no purl ExternalRef"
echo "SBOM libiconv: name=${SBOM_NAME} version=${SBOM_VERSION} license=${SBOM_LICENSE} purl=${SBOM_PURL}"

ZLIB_LICENSE="$(python3 -c "
import json
d = json.load(open('${SBOM}'))
for p in d['packages']:
    if p.get('name') == 'zlib':
        print(p.get('licenseConcluded', 'NONE'))
        break
else:
    print('MISSING')
")"
[ "${ZLIB_LICENSE}" != "MISSING" ] || fail "Trivy SBOM does not contain a zlib package"
echo "SBOM zlib license: ${ZLIB_LICENSE} (sanity check: distinct from libiconv's LGPL identity)"

# ---------------------------------------------------------------------------
log_stage "Build + verify the POSITIVE (linked) target"
# ---------------------------------------------------------------------------
TOOLCHAIN="${SCRIPT_DIR}/build/Release/generators/conan_toolchain.cmake"
[ -f "${TOOLCHAIN}" ] || fail "conan_toolchain.cmake not found at ${TOOLCHAIN} -- did conan install run?"

cmake -S . -B build/integration-linked -G "${GENERATOR}" \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_LGPL_LINK=ON >/dev/null
cmake --build build/integration-linked >/dev/null
LINKED_BINARY="${SCRIPT_DIR}/build/integration-linked/myapp"
[ -x "${LINKED_BINARY}" ] || fail "positive build did not produce ${LINKED_BINARY}"

LINKED_NEEDED="$(readelf -d "${LINKED_BINARY}" | grep NEEDED || true)"
echo "${LINKED_NEEDED}"
echo "${LINKED_NEEDED}" | grep -q "${LIBICONV_SONAME}" \
    || fail "positive binary's DT_NEEDED does not contain ${LIBICONV_SONAME}"
echo "Confirmed: positive binary DT_NEEDED contains ${LIBICONV_SONAME}"

"${LINKED_BINARY}" >/dev/null || fail "positive binary failed to run"
echo "Positive binary runs successfully"

# ---------------------------------------------------------------------------
log_stage "Run Levity against the POSITIVE target"
# ---------------------------------------------------------------------------
POSITIVE_OUTPUT="$("${RBUFF}" "${SBOM}" "${LINKED_BINARY}")"
echo "${POSITIVE_OUTPUT}"
echo "${POSITIVE_OUTPUT}" | grep -q '\[PASS\] IsDynamicallyLinked' \
    || fail "Levity did not report [PASS] IsDynamicallyLinked for the positive case"
echo "${POSITIVE_OUTPUT}" | grep -q '\[FAIL\] IsDynamicallyLinked' \
    && fail "Levity unexpectedly also reported a FAIL for IsDynamicallyLinked in the positive case"
echo "CONFIRMED: IsDynamicallyLinked PASS (real resolver, positive case)"

# ---------------------------------------------------------------------------
log_stage "Build + verify the NEGATIVE (unlinked) target"
# ---------------------------------------------------------------------------
cmake -S . -B build/integration-unlinked -G "${GENERATOR}" \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_LGPL_LINK=OFF >/dev/null
cmake --build build/integration-unlinked >/dev/null
UNLINKED_BINARY="${SCRIPT_DIR}/build/integration-unlinked/myapp"
[ -x "${UNLINKED_BINARY}" ] || fail "negative build did not produce ${UNLINKED_BINARY}"

UNLINKED_NEEDED="$(readelf -d "${UNLINKED_BINARY}" | grep NEEDED || true)"
echo "${UNLINKED_NEEDED}"
if echo "${UNLINKED_NEEDED}" | grep -q "${LIBICONV_SONAME}"; then
    fail "negative binary's DT_NEEDED unexpectedly contains ${LIBICONV_SONAME}"
fi
echo "Confirmed: negative binary DT_NEEDED does NOT contain ${LIBICONV_SONAME}"

"${UNLINKED_BINARY}" >/dev/null || fail "negative binary failed to run"
echo "Negative binary runs successfully"

# ---------------------------------------------------------------------------
log_stage "Run Levity against the NEGATIVE target (same SBOM)"
# ---------------------------------------------------------------------------
NEGATIVE_OUTPUT="$("${RBUFF}" "${SBOM}" "${UNLINKED_BINARY}")"
echo "${NEGATIVE_OUTPUT}"
echo "${NEGATIVE_OUTPUT}" | grep -q '\[FAIL\] IsDynamicallyLinked' \
    || fail "Levity did not report [FAIL] IsDynamicallyLinked for the negative case"
echo "${NEGATIVE_OUTPUT}" | grep '\[FAIL\] IsDynamicallyLinked' | grep -qi "unable to determine" \
    && fail "Levity reported Unresolved (\"unable to determine\") instead of a real NotLinked FAIL -- resolver path is broken"
echo "CONFIRMED: IsDynamicallyLinked FAIL (real resolver, negative case, not Unresolved)"

# ---------------------------------------------------------------------------
log_stage "Regression: existing Levity test suite"
# ---------------------------------------------------------------------------
"${LEVITY_ROOT}/build/tests/rbuff_tests"

log_stage "Integration PASSED"
cat <<EOF

libiconv reference:    ${LIBICONV_REF}
libiconv package ID:   ${LIBICONV_PACKAGE_ID}
libiconv artifact:     ${LIBICONV_SO}
libiconv SONAME:       ${LIBICONV_SONAME}
SBOM purl:             ${SBOM_PURL}
SBOM license:          ${SBOM_LICENSE}
Positive binary:       ${LINKED_BINARY}
Negative binary:       ${UNLINKED_BINARY}
EOF
