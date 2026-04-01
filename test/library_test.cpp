//
// library_test.cpp
// Port of simdhashtest_library.c to Google Test
//

#include <gtest/gtest.h>
#include <cstdint>
#include <cstdlib>
#include <ctime>

extern "C" {
#include "simdhash.h"
#include "simdcommon.h"
#include "hashcommon.h"
}

// Helper: check all 32-bit lanes equal expected
static void ExpectAllLanes32(SimdValue& sv, uint32_t expected) {
    for (size_t i = 0; i < SIMD_WIDTH / 32; i++) {
        EXPECT_EQ(sv.epi32_u32[i], expected) << "Lane " << i;
    }
}

// Helper: check all 64-bit lanes equal expected
static void ExpectAllLanes64(SimdValue& sv, uint64_t expected) {
    for (size_t i = 0; i < SIMD_WIDTH / 64; i++) {
        EXPECT_EQ(sv.epi64_u64[i], expected) << "Lane " << i;
    }
}

// Scalar helpers
static uint32_t RotateRight32(uint32_t v, size_t d) {
    return (v >> d) | (v << (32 - d));
}
static uint32_t RotateLeft32(uint32_t v, size_t d) {
    return (v << d) | (v >> (32 - d));
}

// ============================================================
// SimdLanes
// ============================================================

TEST(Library, SimdLanesMatchesWidth) {
    size_t lanes = SimdLanes();
    EXPECT_EQ(lanes, SIMD_WIDTH / 32);
#if defined(__AVX512BW__) || defined(__AVX512F__)
    EXPECT_EQ(lanes, 16u);
#elif defined(__AVX2__)
    EXPECT_EQ(lanes, 8u);
#elif defined(__SSE2__)
    EXPECT_EQ(lanes, 4u);
#endif
}

// ============================================================
// SIMD arithmetic operations
// ============================================================

TEST(Library, MulEpu32) {
    SimdValue res;
    res.usimd = mul_epu32(set1_epi32(128), set1_epi32(10));
    ExpectAllLanes32(res, 128 * 10);
}

TEST(Library, Mod2Epi32_Exact) {
    SimdValue res;
    res.usimd = mod2_epi32(set1_epi32(1600), 16);
    ExpectAllLanes32(res, 1600 % 16);
}

TEST(Library, Mod2Epi32_Remainder) {
    SimdValue res;
    res.usimd = mod2_epi32(set1_epi32(1601), 16);
    ExpectAllLanes32(res, 1601 % 16);
}

TEST(Library, NotSimd) {
    uint32_t v = 0x41414141;
    SimdValue res;
    res.usimd = not_simd(set1_epi32(v));
    ExpectAllLanes32(res, ~v);
}

// ============================================================
// Shifts
// ============================================================

TEST(Library, SrliEpi32) {
    uint32_t v = 0x12345678;
    SimdValue res;
    res.usimd = srli_epi32(set1_epi32(v), 1);
    ExpectAllLanes32(res, v >> 1);
}

TEST(Library, SlliEpi32_By1) {
    uint32_t v = 0x12345678;
    SimdValue res;
    res.usimd = slli_epi32(set1_epi32(v), 1);
    ExpectAllLanes32(res, v << 1);
}

TEST(Library, SlliEpi32_By8) {
    uint32_t v = 0x12345678;
    SimdValue res;
    res.usimd = slli_epi32(set1_epi32(v), 8);
    ExpectAllLanes32(res, v << 8);
}

TEST(Library, SrliEpi64_By1) {
    uint64_t v = 0x0102030405060708ULL;
    SimdValue res;
    res.usimd = srli_epi64(set1_epi64(v), 1);
    ExpectAllLanes64(res, v >> 1);
}

TEST(Library, SrliEpi64_By8) {
    uint64_t v = 0x0102030405060708ULL;
    SimdValue res;
    res.usimd = srli_epi64(set1_epi64(v), 8);
    ExpectAllLanes64(res, v >> 8);
}

TEST(Library, SlliEpi64_By1) {
    uint64_t v = 0x0102030405060708ULL;
    SimdValue res;
    res.usimd = slli_epi64(set1_epi64(v), 1);
    ExpectAllLanes64(res, v << 1);
}

TEST(Library, SlliEpi64_By8) {
    uint64_t v = 0x0102030405060708ULL;
    SimdValue res;
    res.usimd = slli_epi64(set1_epi64(v), 8);
    ExpectAllLanes64(res, v << 8);
}

// ============================================================
// Rotates
// ============================================================

TEST(Library, RotrEpi32_By1) {
    uint32_t v = 0x12345678;
    SimdValue res;
    res.usimd = rotr_epi32(set1_epi32(v), 1);
    ExpectAllLanes32(res, RotateRight32(v, 1));
}

TEST(Library, RotrEpi32_By8) {
    uint32_t v = 0x12345678;
    SimdValue res;
    res.usimd = rotr_epi32(set1_epi32(v), 8);
    ExpectAllLanes32(res, RotateRight32(v, 8));
}

TEST(Library, RotlEpi32_By1) {
    uint32_t v = 0x12345678;
    SimdValue res;
    res.usimd = rotl_epi32(set1_epi32(v), 1);
    ExpectAllLanes32(res, RotateLeft32(v, 1));
}

TEST(Library, RotlEpi32_By8) {
    uint32_t v = 0x12345678;
    SimdValue res;
    res.usimd = rotl_epi32(set1_epi32(v), 8);
    ExpectAllLanes32(res, RotateLeft32(v, 8));
}

// ============================================================
// SHA256 helper functions (from hashcommon.h)
// ============================================================

static uint32_t ScalarS0(uint32_t a) {
    return RotateRight32(a, 2) ^ RotateRight32(a, 13) ^ RotateRight32(a, 22);
}
static uint32_t ScalarS1(uint32_t e) {
    return RotateRight32(e, 6) ^ RotateRight32(e, 11) ^ RotateRight32(e, 25);
}
static uint32_t ScalarExtendS0(uint32_t w) {
    return RotateRight32(w, 7) ^ RotateRight32(w, 18) ^ (w >> 3);
}
static uint32_t ScalarExtendS1(uint32_t w) {
    return RotateRight32(w, 17) ^ RotateRight32(w, 19) ^ (w >> 10);
}
static uint32_t ScalarCh(uint32_t e, uint32_t f, uint32_t g) {
    return (e & f) ^ ((~e) & g);
}
static uint32_t ScalarMaj(uint32_t a, uint32_t b, uint32_t c) {
    return (a & b) ^ (a & c) ^ (b & c);
}
static uint32_t ScalarTemp1(uint32_t e, uint32_t f, uint32_t g, uint32_t h, uint32_t k, uint32_t w) {
    return h + ScalarS1(e) + ScalarCh(e, f, g) + k + w;
}
static uint32_t ScalarTemp2(uint32_t a, uint32_t b, uint32_t c) {
    return ScalarS0(a) + ScalarMaj(a, b, c);
}

class HashOpsTest : public ::testing::TestWithParam<uint32_t> {};

TEST_P(HashOpsTest, CalculateS0) {
    uint32_t v = GetParam();
    SimdValue res;
    res.usimd = SimdCalculateS0(set1_epi32(v));
    ExpectAllLanes32(res, ScalarS0(v));
}

TEST_P(HashOpsTest, CalculateS1) {
    uint32_t v = GetParam();
    SimdValue res;
    res.usimd = SimdCalculateS1(set1_epi32(v));
    ExpectAllLanes32(res, ScalarS1(v));
}

TEST_P(HashOpsTest, CalculateExtendS0) {
    uint32_t v = GetParam();
    SimdValue res;
    res.usimd = SimdCalculateExtendS0(set1_epi32(v));
    ExpectAllLanes32(res, ScalarExtendS0(v));
}

TEST_P(HashOpsTest, CalculateExtendS1) {
    uint32_t v = GetParam();
    SimdValue res;
    res.usimd = SimdCalculateExtendS1(set1_epi32(v));
    ExpectAllLanes32(res, ScalarExtendS1(v));
}

TEST_P(HashOpsTest, BitwiseChoice) {
    uint32_t v = GetParam();
    simd_t sv = set1_epi32(v);
    SimdValue res;
    res.usimd = SimdBitwiseChoiceWithControl(sv, sv, sv);
    ExpectAllLanes32(res, ScalarCh(v, v, v));
}

TEST_P(HashOpsTest, BitwiseMajority) {
    uint32_t v = GetParam();
    simd_t sv = set1_epi32(v);
    SimdValue res;
    res.usimd = SimdBitwiseMajority(sv, sv, sv);
    ExpectAllLanes32(res, ScalarMaj(v, v, v));
}

TEST_P(HashOpsTest, CalculateTemp1) {
    uint32_t v = GetParam();
    simd_t sv = set1_epi32(v);
    SimdValue res;
    res.usimd = SimdCalculateTemp1(sv, sv, sv, sv, sv, sv);
    ExpectAllLanes32(res, ScalarTemp1(v, v, v, v, v, v));
}

TEST_P(HashOpsTest, CalculateTemp2) {
    uint32_t v = GetParam();
    simd_t sv = set1_epi32(v);
    SimdValue res;
    res.usimd = SimdCalculateTemp2(sv, sv, sv);
    ExpectAllLanes32(res, ScalarTemp2(v, v, v));
}

INSTANTIATE_TEST_SUITE_P(
    HashOps, HashOpsTest,
    ::testing::Values(
        0x00000000, 0xFFFFFFFF, 0x12345678, 0xDEADBEEF,
        0xCAFEBABE, 0x80000001, 0x7FFFFFFF, 0xA5A5A5A5
    )
);
