#include "../src/compression.h"
#include <gtest/gtest.h>

// Test fixture for compression tests
class CompressionTest : public ::testing::Test {
  protected:
    auto SetUp() -> void override {
        // Setup code if needed
    }

    auto TearDown() -> void override {
        // Cleanup code if needed
    }
};

// Test simple string compression and decompression
TEST_F(CompressionTest, SimpleString) {
    const auto input = std::string{"Hello, World!"};

    const auto compressed = compression::compressString(input);
    ASSERT_FALSE(compressed.empty());

    const auto decompressed = compression::decompressData(compressed, input.length());
    EXPECT_EQ(input, decompressed);
}

// Test empty string
TEST_F(CompressionTest, EmptyString) {
    const auto input = std::string{};

    const auto compressed = compression::compressString(input);
    const auto decompressed = compression::decompressData(compressed, input.length());

    EXPECT_EQ(input, decompressed);
}

// Test repeated data (should compress well)
TEST_F(CompressionTest, RepeatedData) {
    const auto input = std::string(1000, 'A');

    const auto compressed = compression::compressString(input);
    ASSERT_FALSE(compressed.empty());

    // Repeated data should compress to much less than original
    EXPECT_LT(compressed.size(), input.length());

    const auto decompressed = compression::decompressData(compressed, input.length());
    EXPECT_EQ(input, decompressed);
}

// Test longer text
TEST_F(CompressionTest, LongerText) {
    const auto input =
        std::string{"Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                    "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
                    "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris. "
                    "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore "
                    "eu fugiat nulla pariatur."};

    const auto compressed = compression::compressString(input);
    ASSERT_FALSE(compressed.empty());

    const auto decompressed = compression::decompressData(compressed, input.length());
    EXPECT_EQ(input, decompressed);
}

// Test random-looking data (won't compress well)
TEST_F(CompressionTest, RandomData) {
    const auto input = std::string{"aB3$xY9@mK7!pQ2#nT8%vZ4^wL6&jR1*hF5+gD0-cS"};

    const auto compressed = compression::compressString(input);
    ASSERT_FALSE(compressed.empty());

    const auto decompressed = compression::decompressData(compressed, input.length());
    EXPECT_EQ(input, decompressed);
}

// Test special characters
TEST_F(CompressionTest, SpecialCharacters) {
    const auto input = std::string{"Special chars: \n\t\r !@#$%^&*(){}[]<>?/|\\\"'`~"};

    const auto compressed = compression::compressString(input);
    ASSERT_FALSE(compressed.empty());

    const auto decompressed = compression::decompressData(compressed, input.length());
    EXPECT_EQ(input, decompressed);
}

// Test unicode characters
TEST_F(CompressionTest, UnicodeCharacters) {
    const auto input = std::string{"Unicode: Hello 世界 🌍 Привет مرحبا"};

    const auto compressed = compression::compressString(input);
    ASSERT_FALSE(compressed.empty());

    const auto decompressed = compression::decompressData(compressed, input.length());
    EXPECT_EQ(input, decompressed);
}

// Test very large data
TEST_F(CompressionTest, LargeData) {
    const auto input = std::string(100000, 'X'); // 100KB of repeated data

    const auto compressed = compression::compressString(input);
    ASSERT_FALSE(compressed.empty());

    // Should compress very well
    EXPECT_LT(compressed.size(), input.length() / 10);

    const auto decompressed = compression::decompressData(compressed, input.length());
    EXPECT_EQ(input, decompressed);
}
