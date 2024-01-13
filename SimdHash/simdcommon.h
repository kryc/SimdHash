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
#include <stdint.h>
#include <immintrin.h>	// AVX

// #ifdef __AVX512F__
// #define __simd __mm512i
// #else
// #define __simd __mm256i
// #endif

typedef union _SimdValue
{
	uint8_t  epi32_u8 [256/32][4];	// Access to each lane as a uint8 array
	uint32_t epi32_u32[256/32];		// Access to each lane as a uint32
	__m256i  u256;
} SimdValue __attribute__((__aligned__(32)));

//
// AVX2 Utilities
//
// static inline
// __m256i
// _mm256_bswap_epi32(
// 	const __m256i Value)
// {
// 	__m256i shuffleMask = _mm256_setr_epi32(
// 		0x00010203,	0x04050607,
// 		0x08090a0b,	0x0c0d0e0f,
// 		0x10111213,	0x14151617,
// 		0x18191a1b,	0x1c1d1e1f
// 	);
// 	return _mm256_shuffle_epi8(Value, shuffleMask);
// }

static inline
__m256i
_mm256_rotl_epi32(
	const __m256i Value,
	const uint8_t Distance)
{
	assert(Distance < 32);
	__m256i shl = _mm256_slli_epi32(Value, Distance);
	__m256i shr = _mm256_srli_epi32(Value, 32 - Distance);
	return _mm256_or_si256(shl, shr);
}

static inline
__m256i
_mm256_rotr_epi32(
	const __m256i Value,
	const uint8_t Distance)
{
	assert(Distance < 32);
	__m256i shr = _mm256_srli_epi32(Value, Distance);
	__m256i shl = _mm256_slli_epi32(Value, 32 - Distance);
	return _mm256_or_si256(shl, shr);
}


//
// AVX512 Utilities
//
static inline
__m512i
_mm512_bswap_epi32(
	const __m512i Value)
{
	__m512i shuffleMask = _mm512_setr_epi32(
		0x00010203, 0x04050607,
		0x08090a0b, 0x0c0d0e0f,
		0x10111213, 0x14151617,
		0x18191a1b,	0x1c1d1e1f,
		0x20212223,	0x24252627,
		0x28292a2b,	0x2c2d2e2f,
		0x30313233,	0x34353637,
		0x38393a3b,	0x3c3d3e3f
	);
	return _mm512_shuffle_epi8(Value, shuffleMask);
}

static inline
__m512i
_mm512_rotl_epi32(
	const __m512i Value,
	const uint8_t Distance)
{
	assert(Distance < 32);
	__m512i shl = _mm512_slli_epi32(Value, Distance);
	__m512i shr = _mm512_srli_epi32(Value, 32 - Distance);
	return _mm512_or_si512(shl, shr);
}

static inline
__m512i
_mm512_rotr_epi32(
	const __m512i Value,
	const uint8_t Distance)
{
	assert(Distance < 32);
	__m512i shr = _mm512_srli_epi32(Value, Distance);
	__m512i shl = _mm512_slli_epi32(Value, 32 - Distance);
	return _mm512_or_si512(shl, shr);
}

#endif /* simdcommon_h */
