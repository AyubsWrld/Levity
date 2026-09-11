#include "elf.hpp"

#include "process_runner.hpp"

#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace ts {

namespace {

// Parses lines readelf -d emits for entries with a bracketed value, e.g.:
//   0x0000000000000001 (NEEDED)  Shared library: [libavcodec.so.58]
//   0x000000000000000e (SONAME) Library soname: [libfmt.so.11]
auto ParseBracketedEntries(const std::string &readelf_output,
                           const std::string &marker)
    -> std::vector<std::string> {
  std::vector<std::string> entries;
  std::istringstream stream(readelf_output);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.find(marker) == std::string::npos) {
      continue;
    }
    auto open_bracket = line.find('[');
    auto close_bracket = line.find(']', open_bracket);
    if (open_bracket == std::string::npos ||
        close_bracket == std::string::npos) {
      continue; // unexpected readelf output shape -- skip rather than crash
    }
    entries.push_back(
        line.substr(open_bracket + 1, close_bracket - open_bracket - 1));
  }
  return entries;
}

} // namespace

auto IsElfFile(const std::filesystem::path &path) -> bool {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return false;
  }
  std::array<unsigned char, 4> magic{};
  file.read(reinterpret_cast<char *>(magic.data()), magic.size());
  if (!file) {
    return false;
  }
  return magic[0] == 0x7f && magic[1] == 'E' && magic[2] == 'L' &&
         magic[3] == 'F';
}

auto GetNeededLibraries(const std::filesystem::path &path)
    -> std::vector<std::string> {
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("target binary does not exist: " +
                             path.string());
  }
  if (!IsElfFile(path)) {
    throw std::runtime_error("target is not an ELF file: " + path.string());
  }

  const std::string output =
      GetProcessRunner()("readelf -d " + path.string() + " 2>&1");
  return ParseBracketedEntries(output, "(NEEDED)");
}

auto GetSoName(const std::filesystem::path &path)
    -> std::optional<std::string> {
  if (!std::filesystem::exists(path) || !IsElfFile(path)) {
    return std::nullopt;
  }

  const std::string output =
      GetProcessRunner()("readelf -d " + path.string() + " 2>&1");
  auto entries = ParseBracketedEntries(output, "(SONAME)");
  if (entries.empty()) {
    return std::nullopt;
  }
  return entries.front();
}

} // namespace ts
