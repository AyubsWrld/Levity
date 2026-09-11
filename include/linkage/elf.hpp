#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace ts {

// ELF metadata extraction. Deliberately backed by `readelf` (via
// ProcessRunner) rather than a from-scratch ELF/DWARF parser -- readelf is
// the standard POSIX tool for this and reimplementing it is out of scope.

// True if `path` exists and starts with the ELF magic number. This is a
// 4-byte check, not full parsing -- just enough to give a clear
// "target not ELF" diagnostic before shelling out to readelf.
[[nodiscard]] auto IsElfFile(const std::filesystem::path &path) -> bool;

// Parses `readelf -d`'s dynamic section for this binary/library and returns
// the SONAME(s) it NEEDS at runtime, e.g. ["libfmt.so.11", "libc.so.6"].
// Throws std::runtime_error if readelf fails to run or the file cannot be
// read as ELF.
[[nodiscard]] auto GetNeededLibraries(const std::filesystem::path &path)
    -> std::vector<std::string>;

// Parses `readelf -d`'s dynamic section for this shared library's own
// SONAME, e.g. "libfmt.so.11". Returns std::nullopt if the library has no
// SONAME entry (not all shared objects set one).
[[nodiscard]] auto GetSoName(const std::filesystem::path &path)
    -> std::optional<std::string>;

} // namespace ts
