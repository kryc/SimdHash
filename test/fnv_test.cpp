//
// fnv_test.cpp
// Tests for FNV-1 and FNV-1a hash (32-bit and 64-bit) SIMD implementations
//

#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include "simdhash.h"
}

// ============================================================
// Known test vectors from the FNV spec
// http://www.isthe.com/chongo/tech/comp/fnv/
// ============================================================

struct FnvTestVector {
    std::string input;
    uint32_t fnv1_32;
    uint32_t fnv1a_32;
    uint64_t fnv1_64;
    uint64_t fnv1a_64;
};

// Verified FNV test vectors
static const std::vector<FnvTestVector> kFnvVectors = {
    {"",         0x811c9dc5, 0x811c9dc5, 0xcbf29ce484222325ULL, 0xcbf29ce484222325ULL},
    {"a",        0x050c5d7e, 0xe40c292c, 0xaf63bd4c8601b7beULL, 0xaf63dc4c8601ec8cULL},
    {"foobar",   0x31f0b262, 0xbf9cf968, 0x340d8765a4dda9c2ULL, 0x85944171f73967e8ULL},
    {"Hello",    0x3726bd47, 0xf55c314b, 0xfa365282a44c0ba7ULL, 0x63f0bfacf2c00f6bULL},
};

// ============================================================
// Test scalar implementations against known vectors
// ============================================================

TEST(FnvScalar, Fnv1_32KnownVectors) {
    for (const auto& tv : kFnvVectors) {
        uint32_t hash;
        Fnv1_32Single((const uint8_t*)tv.input.data(), tv.input.size(), (uint8_t*)&hash);
        EXPECT_EQ(hash, tv.fnv1_32) << "Input: \"" << tv.input << "\"";
    }
}

TEST(FnvScalar, Fnv1a_32KnownVectors) {
    for (const auto& tv : kFnvVectors) {
        uint32_t hash;
        Fnv1a_32Single((const uint8_t*)tv.input.data(), tv.input.size(), (uint8_t*)&hash);
        EXPECT_EQ(hash, tv.fnv1a_32) << "Input: \"" << tv.input << "\"";
    }
}

TEST(FnvScalar, Fnv1_64KnownVectors) {
    for (const auto& tv : kFnvVectors) {
        uint64_t hash;
        Fnv1_64Single((const uint8_t*)tv.input.data(), tv.input.size(), (uint8_t*)&hash);
        EXPECT_EQ(hash, tv.fnv1_64) << "Input: \"" << tv.input << "\"";
    }
}

TEST(FnvScalar, Fnv1a_64KnownVectors) {
    for (const auto& tv : kFnvVectors) {
        uint64_t hash;
        Fnv1a_64Single((const uint8_t*)tv.input.data(), tv.input.size(), (uint8_t*)&hash);
        EXPECT_EQ(hash, tv.fnv1a_64) << "Input: \"" << tv.input << "\"";
    }
}

// ============================================================
// Test SIMD implementations: all lanes same input
// ============================================================

TEST(FnvSimd, Fnv1_32AllLanesSame) {
    size_t lanes = SimdLanes();
    for (const auto& tv : kFnvVectors) {
        const uint8_t* buffers[MAX_LANES];
        size_t lengths[MAX_LANES];
        for (size_t i = 0; i < lanes; i++) {
            buffers[i] = (const uint8_t*)tv.input.data();
            lengths[i] = tv.input.size();
        }
        uint8_t hashes[MAX_LANES * 4];
        SimdHash(HashAlgorithmFNV1_32, lengths, buffers, hashes);

        for (size_t i = 0; i < lanes; i++) {
            uint32_t got = *(uint32_t*)&hashes[i * 4];
            EXPECT_EQ(got, tv.fnv1_32)
                << "Lane " << i << ", input: \"" << tv.input << "\"";
        }
    }
}

TEST(FnvSimd, Fnv1a_32AllLanesSame) {
    size_t lanes = SimdLanes();
    for (const auto& tv : kFnvVectors) {
        const uint8_t* buffers[MAX_LANES];
        size_t lengths[MAX_LANES];
        for (size_t i = 0; i < lanes; i++) {
            buffers[i] = (const uint8_t*)tv.input.data();
            lengths[i] = tv.input.size();
        }
        uint8_t hashes[MAX_LANES * 4];
        SimdHash(HashAlgorithmFNV1a_32, lengths, buffers, hashes);

        for (size_t i = 0; i < lanes; i++) {
            uint32_t got = *(uint32_t*)&hashes[i * 4];
            EXPECT_EQ(got, tv.fnv1a_32)
                << "Lane " << i << ", input: \"" << tv.input << "\"";
        }
    }
}

TEST(FnvSimd, Fnv1_64AllLanesSame) {
    size_t lanes = SimdLanes();
    for (const auto& tv : kFnvVectors) {
        const uint8_t* buffers[MAX_LANES];
        size_t lengths[MAX_LANES];
        for (size_t i = 0; i < lanes; i++) {
            buffers[i] = (const uint8_t*)tv.input.data();
            lengths[i] = tv.input.size();
        }
        // 64-bit: 2 dwords per lane, stored as H[0] and H[1]
        uint8_t hashes[MAX_LANES * 8];
        SimdHash(HashAlgorithmFNV1_64, lengths, buffers, hashes);

        for (size_t i = 0; i < lanes; i++) {
            uint64_t got = *(uint64_t*)&hashes[i * 8];
            EXPECT_EQ(got, tv.fnv1_64)
                << "Lane " << i << ", input: \"" << tv.input << "\""
                << "\nExpected: " << std::hex << tv.fnv1_64
                << "\n  Actual: " << std::hex << got;
        }
    }
}

TEST(FnvSimd, Fnv1a_64AllLanesSame) {
    size_t lanes = SimdLanes();
    for (const auto& tv : kFnvVectors) {
        const uint8_t* buffers[MAX_LANES];
        size_t lengths[MAX_LANES];
        for (size_t i = 0; i < lanes; i++) {
            buffers[i] = (const uint8_t*)tv.input.data();
            lengths[i] = tv.input.size();
        }
        uint8_t hashes[MAX_LANES * 8];
        SimdHash(HashAlgorithmFNV1a_64, lengths, buffers, hashes);

        for (size_t i = 0; i < lanes; i++) {
            uint64_t got = *(uint64_t*)&hashes[i * 8];
            EXPECT_EQ(got, tv.fnv1a_64)
                << "Lane " << i << ", input: \"" << tv.input << "\""
                << "\nExpected: " << std::hex << tv.fnv1a_64
                << "\n  Actual: " << std::hex << got;
        }
    }
}

// ============================================================
// Test SIMD: mixed inputs across lanes
// ============================================================

TEST(FnvSimd, Fnv1_32MixedLanes) {
    size_t lanes = SimdLanes();
    const uint8_t* buffers[MAX_LANES];
    size_t lengths[MAX_LANES];

    for (size_t i = 0; i < lanes; i++) {
        const auto& tv = kFnvVectors[i % kFnvVectors.size()];
        buffers[i] = (const uint8_t*)tv.input.data();
        lengths[i] = tv.input.size();
    }

    uint8_t hashes[MAX_LANES * 4];
    SimdHash(HashAlgorithmFNV1_32, lengths, buffers, hashes);

    for (size_t i = 0; i < lanes; i++) {
        const auto& tv = kFnvVectors[i % kFnvVectors.size()];
        uint32_t got = *(uint32_t*)&hashes[i * 4];
        EXPECT_EQ(got, tv.fnv1_32)
            << "Lane " << i << ", input: \"" << tv.input << "\"";
    }
}

TEST(FnvSimd, Fnv1a_32MixedLanes) {
    size_t lanes = SimdLanes();
    const uint8_t* buffers[MAX_LANES];
    size_t lengths[MAX_LANES];

    for (size_t i = 0; i < lanes; i++) {
        const auto& tv = kFnvVectors[i % kFnvVectors.size()];
        buffers[i] = (const uint8_t*)tv.input.data();
        lengths[i] = tv.input.size();
    }

    uint8_t hashes[MAX_LANES * 4];
    SimdHash(HashAlgorithmFNV1a_32, lengths, buffers, hashes);

    for (size_t i = 0; i < lanes; i++) {
        const auto& tv = kFnvVectors[i % kFnvVectors.size()];
        uint32_t got = *(uint32_t*)&hashes[i * 4];
        EXPECT_EQ(got, tv.fnv1a_32)
            << "Lane " << i << ", input: \"" << tv.input << "\"";
    }
}

TEST(FnvSimd, Fnv1_64MixedLanes) {
    size_t lanes = SimdLanes();
    const uint8_t* buffers[MAX_LANES];
    size_t lengths[MAX_LANES];

    for (size_t i = 0; i < lanes; i++) {
        const auto& tv = kFnvVectors[i % kFnvVectors.size()];
        buffers[i] = (const uint8_t*)tv.input.data();
        lengths[i] = tv.input.size();
    }

    uint8_t hashes[MAX_LANES * 8];
    SimdHash(HashAlgorithmFNV1_64, lengths, buffers, hashes);

    for (size_t i = 0; i < lanes; i++) {
        const auto& tv = kFnvVectors[i % kFnvVectors.size()];
        uint64_t got = *(uint64_t*)&hashes[i * 8];
        EXPECT_EQ(got, tv.fnv1_64)
            << "Lane " << i << ", input: \"" << tv.input << "\"";
    }
}

TEST(FnvSimd, Fnv1a_64MixedLanes) {
    size_t lanes = SimdLanes();
    const uint8_t* buffers[MAX_LANES];
    size_t lengths[MAX_LANES];

    for (size_t i = 0; i < lanes; i++) {
        const auto& tv = kFnvVectors[i % kFnvVectors.size()];
        buffers[i] = (const uint8_t*)tv.input.data();
        lengths[i] = tv.input.size();
    }

    uint8_t hashes[MAX_LANES * 8];
    SimdHash(HashAlgorithmFNV1a_64, lengths, buffers, hashes);

    for (size_t i = 0; i < lanes; i++) {
        const auto& tv = kFnvVectors[i % kFnvVectors.size()];
        uint64_t got = *(uint64_t*)&hashes[i * 8];
        EXPECT_EQ(got, tv.fnv1a_64)
            << "Lane " << i << ", input: \"" << tv.input << "\"";
    }
}

// ============================================================
// SIMD vs scalar comparison with random-ish inputs
// ============================================================

TEST(FnvSimd, Fnv1_32SimdVsScalar) {
    size_t lanes = SimdLanes();
    const std::string inputs[] = {
        "test", "password", "12345", "hello world",
        "The quick brown fox", "x", "", "abcdefghijklmnop"
    };

    const uint8_t* buffers[MAX_LANES];
    size_t lengths[MAX_LANES];
    for (size_t i = 0; i < lanes; i++) {
        const auto& s = inputs[i % 8];
        buffers[i] = (const uint8_t*)s.data();
        lengths[i] = s.size();
    }

    uint8_t hashes[MAX_LANES * 4];
    SimdHash(HashAlgorithmFNV1_32, lengths, buffers, hashes);

    for (size_t i = 0; i < lanes; i++) {
        uint32_t expected;
        Fnv1_32Single(buffers[i], lengths[i], (uint8_t*)&expected);
        uint32_t got = *(uint32_t*)&hashes[i * 4];
        EXPECT_EQ(got, expected) << "Lane " << i;
    }
}

TEST(FnvSimd, Fnv1a_32SimdVsScalar) {
    size_t lanes = SimdLanes();
    const std::string inputs[] = {
        "test", "password", "12345", "hello world",
        "The quick brown fox", "x", "", "abcdefghijklmnop"
    };

    const uint8_t* buffers[MAX_LANES];
    size_t lengths[MAX_LANES];
    for (size_t i = 0; i < lanes; i++) {
        const auto& s = inputs[i % 8];
        buffers[i] = (const uint8_t*)s.data();
        lengths[i] = s.size();
    }

    uint8_t hashes[MAX_LANES * 4];
    SimdHash(HashAlgorithmFNV1a_32, lengths, buffers, hashes);

    for (size_t i = 0; i < lanes; i++) {
        uint32_t expected;
        Fnv1a_32Single(buffers[i], lengths[i], (uint8_t*)&expected);
        uint32_t got = *(uint32_t*)&hashes[i * 4];
        EXPECT_EQ(got, expected) << "Lane " << i;
    }
}

TEST(FnvSimd, Fnv1_64SimdVsScalar) {
    size_t lanes = SimdLanes();
    const std::string inputs[] = {
        "test", "password", "12345", "hello world",
        "The quick brown fox", "x", "", "abcdefghijklmnop"
    };

    const uint8_t* buffers[MAX_LANES];
    size_t lengths[MAX_LANES];
    for (size_t i = 0; i < lanes; i++) {
        const auto& s = inputs[i % 8];
        buffers[i] = (const uint8_t*)s.data();
        lengths[i] = s.size();
    }

    uint8_t hashes[MAX_LANES * 8];
    SimdHash(HashAlgorithmFNV1_64, lengths, buffers, hashes);

    for (size_t i = 0; i < lanes; i++) {
        uint64_t expected;
        Fnv1_64Single(buffers[i], lengths[i], (uint8_t*)&expected);
        uint64_t got = *(uint64_t*)&hashes[i * 8];
        EXPECT_EQ(got, expected) << "Lane " << i;
    }
}

TEST(FnvSimd, Fnv1a_64SimdVsScalar) {
    size_t lanes = SimdLanes();
    const std::string inputs[] = {
        "test", "password", "12345", "hello world",
        "The quick brown fox", "x", "", "abcdefghijklmnop"
    };

    const uint8_t* buffers[MAX_LANES];
    size_t lengths[MAX_LANES];
    for (size_t i = 0; i < lanes; i++) {
        const auto& s = inputs[i % 8];
        buffers[i] = (const uint8_t*)s.data();
        lengths[i] = s.size();
    }

    uint8_t hashes[MAX_LANES * 8];
    SimdHash(HashAlgorithmFNV1a_64, lengths, buffers, hashes);

    for (size_t i = 0; i < lanes; i++) {
        uint64_t expected;
        Fnv1a_64Single(buffers[i], lengths[i], (uint8_t*)&expected);
        uint64_t got = *(uint64_t*)&hashes[i * 8];
        EXPECT_EQ(got, expected) << "Lane " << i;
    }
}
