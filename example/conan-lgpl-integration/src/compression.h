#pragma once

#include <string>
#include <vector>
#include <cstddef>

namespace compression {

// Compress a string using zlib
[[nodiscard]] auto compressString(const std::string& input) -> std::vector<unsigned char>;

// Decompress data back to a string
[[nodiscard]] auto decompressData(const std::vector<unsigned char>& compressed, std::size_t originalSize) -> std::string;

} // namespace compression

