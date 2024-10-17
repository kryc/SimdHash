//
//  simdcommon.h
//  SimdHash
//
//  Created by Gareth Evans on 20/01/2021.
//  Copyright © 2021 Gareth Evans. All rights reserved.
//

#ifndef simdcommon_h
#define simdcommon_h

#include <assert.h>
#include <math.h>
#include <stdint.h>

#ifdef __AVX2__
#include <immintrin.h>
#elif defined __arm64__
#include <arm_neon.h>
#endif

#ifdef __AVX512F__
#define simd_t __m512i
#define SIMD_WIDTH   512
#define load_simd       _mm512_load_si512
#define store_simd      _mm512_store_si512
#define set1_epi32      _mm512_set1_epi32
#define set1_epi64      _mm512_set1_epi64
#define shuffle_epi8    _mm512_shuffle_epi8
#define add_epi32       _mm512_add_epi32
#define sub_epi32       _mm512_sub_epi32
#define mul_epu32       _mm512_mul_epu32
#define srli_epi32      _mm512_srli_epi32
#define srli_epi64		_mm512_srli_epi64
#define slli_epi32      _mm512_slli_epi32
#define slli_epi64      _mm512_slli_epi64
#define xor_simd        _mm512_xor_si512
#define or_simd         _mm512_or_si512
#define and_simd        _mm512_and_si512
#define andnot_simd     _mm512_andnot_si512
#define cmpeq_epi32     _mm512_cmpeq_epi32
// Custom
#define bswap_epi32     _mm512_bswap_epi32
#elif defined __arm64__
#define simd_t          uint32x4_t
#define SIMD_WIDTH      128
#define load_simd(x)    vld1q_u32((uint32_t*)(x))
#define store_simd(x,v) vst1q_u32((uint32_t*)(x), (v))
#define set1_epi32      vdupq_n_u32
#define set1_epi64      vdupq_n_u64
#define add_epi32       vaddq_u32
#define sub_epi32       vsubq_u32
#define mul_epu32       vmulq_u32
#define xor_simd        veorq_u32
#define or_simd         vorrq_u32
#define and_simd        vandq_u32
#define not_simd        vmvnq_u32
#define andnot_simd     andnot_simd_custom
#define cmpeq_simd      vceq_u32
#else // AVX2
#define simd_t __m256i
#define SIMD_WIDTH      256
#define load_simd       _mm256_load_si256
#define store_simd      _mm256_store_si256
#define set1_epi32      _mm256_set1_epi32
#define set1_epi64      _mm256_set1_epi64
#define shuffle_epi8    _mm256_shuffle_epi8
#define add_epi32       _mm256_add_epi32
#define sub_epi32       _mm256_sub_epi32
#define mul_epu32       _mm256_mul_epu32
#define srli_epi32      _mm256_srli_epi32
#define srli_epi64      _mm256_srli_epi64
#define slli_epi32      _mm256_slli_epi32
#define slli_epi64      _mm256_slli_epi64
#define xor_simd        _mm256_xor_si256
#define or_simd         _mm256_or_si256
#define and_simd        _mm256_and_si256
#define andnot_simd     _mm256_andnot_si256
#define cmpeq_epi32     _mm256_cmpeq_epi32
// Custom
#define bswap_epi32     _mm256_bswap_epi32
#endif

#define MAX_LANES (512/32)

#ifdef __arm64__
/*
 * ARM64 doesn't have shift by const value,
 * so we need to mimick the x86 version here
 */
static inline
simd_t
srli_epi32(
	const simd_t Value,
	const uint8_t Distance
)
{
	assert(Distance < 32);
	const simd_t d = set1_epi32(Distance);
	return vshlq_u32(Value, vnegq_s32(vreinterpretq_s32_u32(d)));;
}

static inline
simd_t
srli_epi64(
	const simd_t Value,
	const uint8_t Distance
)
{
	assert(Distance < 64);
	const simd_t d = set1_epi64(Distance);
	return vshlq_u64(Value, vnegq_s64(vreinterpretq_s64_u64(d)));;
}

static inline
simd_t
slli_epi32(
	const simd_t Value,
	const uint8_t Distance
)
{
	assert(Distance < 32);
	const simd_t d = set1_epi32(Distance);
	return vshlq_u32(Value, d);
}

static inline
simd_t
slli_epi64(
	const simd_t Value,
	const uint8_t Distance
)
{
	assert(Distance < 64);
	const simd_t d = set1_epi64(Distance);
	return vshlq_u64(Value, d);
}
#else
/*
 * ARM64 brings a native not so no
 * need to redefine it here
 */
static inline
simd_t
not_simd(
	const simd_t Value
)
{
	return xor_simd(Value, set1_epi32(-1));
}
#endif

static inline
simd_t
rotl_epi32(
	const simd_t Value,
	const int Distance)
{
	assert(Distance < 32);
	simd_t shl = slli_epi32(Value, Distance);
	simd_t shr = srli_epi32(Value, 32 - Distance);
	return or_simd(shl, shr);
}

static inline
simd_t
rotr_epi32(
	const simd_t Value,
	const int Distance)
{
	assert(Distance < 32);
	simd_t shr = srli_epi32(Value, Distance);
	simd_t shl = slli_epi32(Value, 32 - Distance);
	return or_simd(shl, shr);
}

static inline
simd_t
andnot_simd_custom(
	const simd_t Value1,
	const simd_t Value2
)
{
	return and_simd(not_simd(Value1), Value2);
}

static inline
simd_t
xmul_epu32(
	const simd_t Value1,
	const simd_t Value2
)
{
	simd_t res_lo = mul_epu32(Value1, Value2);
	simd_t res_hi = mul_epu32(srli_epi64(Value1, 32), srli_epi64(Value2, 32));
	return or_simd(slli_epi64(res_hi, 32), res_lo);
}

static inline
simd_t
mod2_epi32(
	const simd_t Value1,
	const uint32_t Mod2
)
{
	int count = log2(Mod2);
	simd_t quotient = srli_epi32(Value1, count);	// Division
	simd_t remainder = sub_epi32(Value1, xmul_epu32(set1_epi32(Mod2), quotient));
	return remainder;
}

static inline
simd_t
bswap_epi32(
	const simd_t Value)
{

#ifdef __AVX512__
	simd_t shuffleMask = _mm512_setr_epi64(
		0x0001020304050607,
		0x08090a0b0c0d0e0f,
		0x1011121314151617,
		0x18191a1b1c1d1e1f,
		0x2021222324252627,
		0x28292a2b2c2d2e2f,
		0x3031323334353637,
		0x38393a3b3c3d3e3f
	);
	return _mm512_shuffle_epi8(Value, shuffleMask);
#elif defined __arm64__
	return vrev32q_u8(vreinterpretq_u8_u32(Value));
#else // AVX2
	simd_t shuffleMask = _mm256_setr_epi32(
		0x00010203,	0x04050607,
		0x08090a0b,	0x0c0d0e0f,
		0x10111213,	0x14151617,
		0x18191a1b,	0x1c1d1e1f
	);
	return _mm256_shuffle_epi8(Value, shuffleMask);
#endif
}

#endif /* simdcommon_h */
