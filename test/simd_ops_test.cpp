//
// simd_ops_test.cpp
// Comprehensive unit tests for all SIMD operations in simdcommon.h
//

#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <cmath>

extern "C" {
#include "simdhash.h"
#include "simdcommon.h"
}

// Helper: store SIMD value and check all 32-bit lanes equal expected
static void ExpectAllLanes32(simd_t v, uint32_t expected) {
    SimdValue sv;
    store_simd(&sv.usimd, v);
    for (size_t i = 0; i < SIMD_WIDTH / 32; i++) {
        EXPECT_EQ(sv.epi32_u32[i], expected) << "Lane " << i;
    }
}

// Helper: store SIMD value and check all 64-bit lanes equal expected
static void ExpectAllLanes64(simd_t v, uint64_t expected) {
    SimdValue sv;
    store_simd(&sv.usimd, v);
    for (size_t i = 0; i < SIMD_WIDTH / 64; i++) {
        EXPECT_EQ(sv.epi64_u64[i], expected) << "Lane " << i;
    }
}

// Scalar helpers for comparison
static uint32_t RotateLeft32(uint32_t v, int d) {
    return (v << d) | (v >> (32 - d));
}
static uint32_t RotateRight32(uint32_t v, int d) {
    return (v >> d) | (v << (32 - d));
}
static uint32_t ByteSwap32(uint32_t v) {
    return ((v >> 24) & 0xFF) |
           ((v >> 8)  & 0xFF00) |
           ((v << 8)  & 0xFF0000) |
           ((v << 24) & 0xFF000000);
}

// ============================================================
// SimdLanes
// ============================================================

TEST(SimdOps, SimdLanes) {
    EXPECT_EQ(SimdLanes(), SIMD_WIDTH / 32);
}

// ============================================================
// set1 / load / store
// ============================================================

TEST(SimdOps, Set1Epi32) {
    ExpectAllLanes32(set1_epi32(0x12345678), 0x12345678);
    ExpectAllLanes32(set1_epi32(0), 0);
    ExpectAllLanes32(set1_epi32(0xFFFFFFFF), 0xFFFFFFFF);
}

TEST(SimdOps, Set1Epi64) {
    ExpectAllLanes64(set1_epi64(0x0102030405060708ULL), 0x0102030405060708ULL);
}

TEST(SimdOps, LoadStore) {
    SimdValue sv;
    for (size_t i = 0; i < SIMD_WIDTH / 32; i++) {
        sv.epi32_u32[i] = (uint32_t)(i * 0x11111111);
    }
    simd_t loaded = load_simd(&sv.usimd);
    SimdValue sv2;
    store_simd(&sv2.usimd, loaded);
    EXPECT_EQ(0, memcmp(&sv, &sv2, SIMD_WIDTH / 8));
}

// ============================================================
// Arithmetic: add, sub
// ============================================================

TEST(SimdOps, AddEpi32) {
    ExpectAllLanes32(add_epi32(set1_epi32(100), set1_epi32(200)), 300);
    // Overflow wraps
    ExpectAllLanes32(add_epi32(set1_epi32(0xFFFFFFFF), set1_epi32(1)), 0);
}

TEST(SimdOps, SubEpi32) {
    ExpectAllLanes32(sub_epi32(set1_epi32(300), set1_epi32(100)), 200);
    // Underflow wraps
    ExpectAllLanes32(sub_epi32(set1_epi32(0), set1_epi32(1)), 0xFFFFFFFF);
}

// ============================================================
// Shifts: srli/slli epi32 and epi64
// ============================================================

TEST(SimdOps, SrliEpi32) {
    uint32_t v = 0x12345678;
    ExpectAllLanes32(srli_epi32(set1_epi32(v), 1), v >> 1);
    ExpectAllLanes32(srli_epi32(set1_epi32(v), 8), v >> 8);
    ExpectAllLanes32(srli_epi32(set1_epi32(v), 16), v >> 16);
    ExpectAllLanes32(srli_epi32(set1_epi32(v), 31), v >> 31);
}

TEST(SimdOps, SlliEpi32) {
    uint32_t v = 0x12345678;
    ExpectAllLanes32(slli_epi32(set1_epi32(v), 1), v << 1);
    ExpectAllLanes32(slli_epi32(set1_epi32(v), 8), v << 8);
    ExpectAllLanes32(slli_epi32(set1_epi32(v), 16), v << 16);
    ExpectAllLanes32(slli_epi32(set1_epi32(v), 31), v << 31);
}

TEST(SimdOps, SrliEpi64) {
    uint64_t v = 0x0102030405060708ULL;
    ExpectAllLanes64(srli_epi64(set1_epi64(v), 1), v >> 1);
    ExpectAllLanes64(srli_epi64(set1_epi64(v), 8), v >> 8);
    ExpectAllLanes64(srli_epi64(set1_epi64(v), 32), v >> 32);
}

TEST(SimdOps, SlliEpi64) {
    uint64_t v = 0x0102030405060708ULL;
    ExpectAllLanes64(slli_epi64(set1_epi64(v), 1), v << 1);
    ExpectAllLanes64(slli_epi64(set1_epi64(v), 8), v << 8);
    ExpectAllLanes64(slli_epi64(set1_epi64(v), 32), v << 32);
}

// ============================================================
// Bitwise: xor, or, and, andnot, not
// ============================================================

TEST(SimdOps, XorSimd) {
    ExpectAllLanes32(xor_simd(set1_epi32(0xFF00FF00), set1_epi32(0x0FF00FF0)), 0xF0F0F0F0);
    // XOR with self = 0
    ExpectAllLanes32(xor_simd(set1_epi32(0x12345678), set1_epi32(0x12345678)), 0);
}

TEST(SimdOps, OrSimd) {
    ExpectAllLanes32(or_simd(set1_epi32(0xF0F0F0F0), set1_epi32(0x0F0F0F0F)), 0xFFFFFFFF);
    ExpectAllLanes32(or_simd(set1_epi32(0), set1_epi32(0x12345678)), 0x12345678);
}

TEST(SimdOps, AndSimd) {
    ExpectAllLanes32(and_simd(set1_epi32(0xFF00FF00), set1_epi32(0x0FF00FF0)), 0x0F000F00);
    ExpectAllLanes32(and_simd(set1_epi32(0xFFFFFFFF), set1_epi32(0x12345678)), 0x12345678);
}

TEST(SimdOps, AndnotSimd) {
    // andnot(a, b) = (~a) & b
    ExpectAllLanes32(andnot_simd(set1_epi32(0xFF00FF00), set1_epi32(0xFFFFFFFF)), 0x00FF00FF);
    ExpectAllLanes32(andnot_simd(set1_epi32(0), set1_epi32(0x12345678)), 0x12345678);
}

TEST(SimdOps, NotSimd) {
    ExpectAllLanes32(not_simd(set1_epi32(0x41414141)), ~(uint32_t)0x41414141);
    ExpectAllLanes32(not_simd(set1_epi32(0)), 0xFFFFFFFF);
    ExpectAllLanes32(not_simd(set1_epi32(0xFFFFFFFF)), 0);
}

// ============================================================
// Rotates: rotl_epi32, rotr_epi32
// ============================================================

TEST(SimdOps, RotlEpi32) {
    uint32_t v = 0x12345678;
    for (int d = 1; d < 32; d++) {
        ExpectAllLanes32(rotl_epi32(set1_epi32(v), d), RotateLeft32(v, d));
    }
}

TEST(SimdOps, RotrEpi32) {
    uint32_t v = 0x12345678;
    for (int d = 1; d < 32; d++) {
        ExpectAllLanes32(rotr_epi32(set1_epi32(v), d), RotateRight32(v, d));
    }
}

// ============================================================
// mul_epu32
// ============================================================

TEST(SimdOps, MulEpu32) {
    ExpectAllLanes32(mul_epu32(set1_epi32(128), set1_epi32(10)), 1280);
    ExpectAllLanes32(mul_epu32(set1_epi32(1), set1_epi32(1)), 1);
    ExpectAllLanes32(mul_epu32(set1_epi32(0), set1_epi32(999)), 0);
    ExpectAllLanes32(mul_epu32(set1_epi32(7), set1_epi32(13)), 91);
    ExpectAllLanes32(mul_epu32(set1_epi32(255), set1_epi32(256)), 65280);
}

// ============================================================
// mod2_epi32 / shift_mod2_epi32
// ============================================================

TEST(SimdOps, Mod2Epi32) {
    ExpectAllLanes32(mod2_epi32(set1_epi32(1600), 16), 1600 % 16);
    ExpectAllLanes32(mod2_epi32(set1_epi32(1601), 16), 1601 % 16);
    ExpectAllLanes32(mod2_epi32(set1_epi32(255), 8), 255 % 8);
    ExpectAllLanes32(mod2_epi32(set1_epi32(256), 256), 0);
}

TEST(SimdOps, ShiftMod2Epi32) {
    // shift_mod2 takes the log2 directly
    ExpectAllLanes32(shift_mod2_epi32(set1_epi32(1600), 4), 1600 % 16); // 2^4 = 16
    ExpectAllLanes32(shift_mod2_epi32(set1_epi32(100), 3), 100 % 8);   // 2^3 = 8
}

// ============================================================
// bswap_epi32
// ============================================================

TEST(SimdOps, BswapEpi32) {
    ExpectAllLanes32(bswap_epi32(set1_epi32(0x12345678)), ByteSwap32(0x12345678));
    ExpectAllLanes32(bswap_epi32(set1_epi32(0x00000000)), 0);
    ExpectAllLanes32(bswap_epi32(set1_epi32(0xFFFFFFFF)), 0xFFFFFFFF);
    ExpectAllLanes32(bswap_epi32(set1_epi32(0xAABBCCDD)), ByteSwap32(0xAABBCCDD));
    // Double bswap = identity
    ExpectAllLanes32(bswap_epi32(bswap_epi32(set1_epi32(0xDEADBEEF))), 0xDEADBEEF);
}

// ============================================================
// cmpeq_epi32
// ============================================================

TEST(SimdOps, CmpeqEpi32) {
#if defined(__AVX512F__)
    // AVX-512 cmpeq returns __mmask16
    EXPECT_EQ(cmpeq_epi32(set1_epi32(42), set1_epi32(42)), (__mmask16)0xFFFF);
    EXPECT_EQ(cmpeq_epi32(set1_epi32(42), set1_epi32(43)), (__mmask16)0);
#else
    // Equal: all bits set
    ExpectAllLanes32(cmpeq_epi32(set1_epi32(42), set1_epi32(42)), 0xFFFFFFFF);
    // Not equal: all bits clear
    ExpectAllLanes32(cmpeq_epi32(set1_epi32(42), set1_epi32(43)), 0);
#endif
}

// ============================================================
// andnot_simd_custom (always available)
// ============================================================

TEST(SimdOps, AndnotSimdCustom) {
    ExpectAllLanes32(andnot_simd_custom(set1_epi32(0xFF00FF00), set1_epi32(0xFFFFFFFF)), 0x00FF00FF);
}

// ============================================================
// Combined operations: verify composability
// ============================================================

TEST(SimdOps, CombinedShiftAndMask) {
    // (v >> 8) & 0xFF should extract the second byte
    uint32_t v = 0xAABBCCDD;
    simd_t shifted = srli_epi32(set1_epi32(v), 8);
    simd_t masked = and_simd(shifted, set1_epi32(0xFF));
    ExpectAllLanes32(masked, (v >> 8) & 0xFF);
}

TEST(SimdOps, CombinedRotateXor) {
    uint32_t v = 0x12345678;
    uint32_t expected = RotateRight32(v, 7) ^ RotateRight32(v, 18) ^ (v >> 3);
    simd_t sv = set1_epi32(v);
    simd_t result = xor_simd(xor_simd(rotr_epi32(sv, 7), rotr_epi32(sv, 18)), srli_epi32(sv, 3));
    ExpectAllLanes32(result, expected);
}

// ============================================================
// Edge cases
// ============================================================

TEST(SimdOps, ZeroShifts) {
    uint32_t v = 0xDEADBEEF;
    // Shift by 0 should be identity (implementation-defined for some, but works on all our targets)
    // We skip 0-shifts as the assert(Distance < 32) with Distance > 0 is the intended usage
    // Instead test shift by 1 (minimum)
    ExpectAllLanes32(srli_epi32(set1_epi32(v), 1), v >> 1);
    ExpectAllLanes32(slli_epi32(set1_epi32(v), 1), v << 1);
}

TEST(SimdOps, AllOnesAllZeros) {
    ExpectAllLanes32(and_simd(set1_epi32(0xFFFFFFFF), set1_epi32(0)), 0);
    ExpectAllLanes32(or_simd(set1_epi32(0xFFFFFFFF), set1_epi32(0)), 0xFFFFFFFF);
    ExpectAllLanes32(xor_simd(set1_epi32(0xFFFFFFFF), set1_epi32(0xFFFFFFFF)), 0);
}
