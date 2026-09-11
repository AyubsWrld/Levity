#include "compression.h"
#include <spdlog/spdlog.h>
#include <string_view>
#include <zlib.h>

#if LEVITY_ENABLE_LGPL_LINK
#include "lgpl_link.h"
#endif

auto main() -> int {
    spdlog::info("=== Zlib Compression Demo ===");
    spdlog::info("Zlib version: {}", zlibVersion());

    // Example usage of compression utilities
    constexpr auto sampleTextView =
        std::string_view{"Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                         "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
                         "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris."};
    const auto sampleText = std::string{sampleTextView};

    spdlog::info("Original text ({} bytes): {}", sampleText.length(), sampleText);

    // Compress the text
    const auto compressed = compression::compressString(sampleText);
    if (compressed.empty()) {
        spdlog::error("✗ Compression failed");
        return 1;
    }

    const auto compressionRatio = (compressed.size() * 100.0) / sampleText.length();
    spdlog::info("✓ Compression successful: {} bytes -> {} bytes ({:.2f}% of original)",
                 sampleText.length(), compressed.size(), compressionRatio);

    // Decompress the text
    const auto decompressed = compression::decompressData(compressed, sampleText.length());
    if (decompressed != sampleText) {
        spdlog::error("✗ Decompression failed: data mismatch");
        return 1;
    }

    spdlog::info("✓ Decompression successful: data integrity verified");

#if LEVITY_ENABLE_LGPL_LINK
    if (!lgpl_link::ExerciseIconv()) {
        spdlog::error("✗ libiconv exercise failed");
        return 1;
    }
#else
    spdlog::info("LGPL link disabled at build time (ENABLE_LGPL_LINK=OFF): libiconv not used");
#endif

    spdlog::info("===========================");
    spdlog::info("✓ Application completed successfully!");
    spdlog::info("Run the test suite with './myapp_tests' for comprehensive testing");

    return 0;
}
