#include "compression.h"
#include <algorithm>
#include <spdlog/spdlog.h>
#include <zlib.h>

namespace compression {

auto compressString(const std::string &input) -> std::vector<unsigned char> {
    const auto sourceLen = static_cast<uLong>(input.length());
    auto destLen = compressBound(sourceLen);
    auto compressed = std::vector<unsigned char>(destLen);

    const auto *sourceData = reinterpret_cast<const unsigned char *>(input.data());
    const auto result = compress(compressed.data(), &destLen, sourceData, sourceLen);

    if (result != Z_OK) {
        spdlog::error("Compression failed with error code: {}", result);
        return {};
    }

    compressed.resize(destLen);
    spdlog::debug("Compressed {} bytes to {} bytes (ratio: {:.2f}%)", sourceLen, destLen,
                  (destLen * 100.0) / sourceLen);

    return compressed;
}

auto decompressData(const std::vector<unsigned char> &compressed, std::size_t originalSize)
    -> std::string {
    auto decompressed = std::vector<unsigned char>(originalSize);
    auto destLen = static_cast<uLong>(originalSize);

    const auto result = uncompress(decompressed.data(), &destLen, compressed.data(),
                                   static_cast<uLong>(compressed.size()));

    if (result != Z_OK) {
        spdlog::error("Decompression failed with error code: {}", result);
        return {};
    }

    return std::string(decompressed.cbegin(), decompressed.cbegin() + destLen);
}

} // namespace compression
