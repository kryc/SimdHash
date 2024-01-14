//
//  simdhash.h
//  SimdHash
//
//  Created by Gareth Evans on 26/08/2020.
//  Copyright © 2020 Gareth Evans. All rights reserved.
//

#ifndef simdhash_h
#define simdhash_h

#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>	// AVX

#include "common.h"
#include "simdcommon.h"

#define SIMD_COUNT (SIMD_WIDTH / 32)
#define SHA1_BUFFER_SIZE (64)
#define SHA1_BUFFER_SIZE_DWORDS (SHA1_BUFFER_SIZE / 4)
#define SHA1_H_COUNT (5)
#define SHA1_SIZE (SHA1_H_COUNT * 4)
#define SHA1_MESSAGE_SCHEDULE_SIZE (320)
#define SHA1_MESSAGE_SCHEDULE_SIZE_DWORDS (SHA1_MESSAGE_SCHEDULE_SIZE / 4)
#define SHA256_BUFFER_SIZE (64)
#define SHA256_BUFFER_SIZE_DWORDS (SHA256_BUFFER_SIZE / 4)
#define SHA256_H_COUNT (8)
#define SHA256_SIZE (SHA256_H_COUNT * 4)
#define SHA256_MESSAGE_SCHEDULE_SIZE (256)
#define SHA256_MESSAGE_SCHEDULE_SIZE_DWORDS (SHA256_MESSAGE_SCHEDULE_SIZE / 4)

#define MAX_H_COUNT SHA256_H_COUNT
#define MAX_BUFFER_SIZE SHA256_BUFFER_SIZE
#define MAX_BUFFER_SIZE_DWORDS (MAX_BUFFER_SIZE / 4)

#ifdef __cplusplus
extern "C" {
#endif

typedef union _SimdValue
{
	uint8_t  epi32_u8 [SIMD_WIDTH/32][4];	// Access to each lane as a uint8 array
	uint32_t epi32_u32[SIMD_WIDTH/32];		// Access to each lane as a uint32
	simd_t   usimd;
} SimdValue __attribute__((__aligned__(32)));

typedef struct _SimdShaContext
{
	SimdValue H[MAX_H_COUNT];
	SimdValue Buffer[MAX_BUFFER_SIZE_DWORDS];
	size_t    HSize;
	size_t    BufferSize;
	uint64_t  Length;
	uint64_t  BitLength;
	size_t    Lanes;
} SimdShaContext, *PSimdSha2Context;

void SimdSha256Init(
	SimdShaContext* Context,
	const size_t Lanes);

void SimdSha256Update(
	SimdShaContext* Context,
	const size_t Length,
	const uint8_t* Buffers[]);

void SimdSha256Finalize(
	SimdShaContext* Context);

void SimdSha256GetHashes2D(
	SimdShaContext* Context,
	uint8_t** HashBuffers);

void SimdSha256GetHashes(
	SimdShaContext* Context,
	uint8_t* HashBuffers);

void SimdSha256GetHash(
	SimdShaContext* Context,
	uint8_t* HashBuffer,
	const size_t Lane);

void SimdSha1Init(
	SimdShaContext* Context,
	const size_t Lanes);

void SimdSha1Update(
	SimdShaContext* Context,
	const size_t Length,
	const uint8_t* Buffers[]);

void SimdSha1Finalize(
	SimdShaContext* Context);

void SimdSha1GetHashes(
	SimdShaContext* Context,
	uint8_t** HashBuffers);

void SimdSha256GetHashesUnrolled(
	SimdShaContext* Context,
	uint8_t* HashBuffers);

void SimdSha1GetHash(
	SimdShaContext* Context,
	uint8_t* HashBuffer,
	const size_t Lane);

#ifdef TEST
simd_t SimdCalculateS0(
	const simd_t A);
simd_t SimdCalculateS1(
	const simd_t E);
simd_t SimdCalculateTemp1(
	const simd_t E,
	const simd_t F,
	const simd_t G,
	const simd_t H,
	const simd_t K,
	const simd_t W);
simd_t SimdCalculateTemp2(
	const simd_t A,
	const simd_t B,
	const simd_t C);
#endif

#ifdef __cplusplus
}
#endif

#endif /* simdhash_h */
