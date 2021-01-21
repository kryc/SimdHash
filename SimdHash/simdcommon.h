//
//  simdcommon.h
//  SimdHash
//
//  Created by Gareth Evans on 20/01/2021.
//  Copyright © 2021 Gareth Evans. All rights reserved.
//

#ifndef simdcommon_h
#define simdcommon_h

#include <immintrin.h>	// AVX

static inline
__m256i
_mm256_bswap_epi32(
	const __m256i Value)
{
	__m256i shuffleMask = _mm256_setr_epi8(3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12,19,18,17,16,23,22,21,20,27,26,25,24,31,30,29,28);
	return _mm256_shuffle_epi8(Value, shuffleMask);
}

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

#endif /* simdcommon_h */
