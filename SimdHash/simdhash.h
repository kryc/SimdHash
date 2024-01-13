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

#define SIMD_COUNT 8
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

#ifdef _MSC_VER
  // MSVC...
  #define ALIGN(n) declspec(align(n))
#else
  // the civilised world...
  #define ALIGN(n) __attribute__ ((aligned(n)))
#endif

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct _SimdSha2SecondPreimageContext
{
	SimdShaContext ShaContext;
	uint8_t 		Target8[SHA256_SIZE];
	uint32_t 		Target32[SHA256_SIZE/4];
} SimdSha2SecondPreimageContext, *PSimdSha2SecondPreimageContext;

void SimdSha256Init(
	SimdShaContext* Context,
	const size_t Lanes);

void SimdSha256Update(
	SimdShaContext* Context,
	const size_t Length,
	const uint8_t* Buffers[]);

void SimdSha256Finalize(
	SimdShaContext* Context);

void SimdSha256SecondPreimageInit(
	SimdSha2SecondPreimageContext* Context,
	SimdShaContext* ShaContext,
	const uint8_t* Target);

size_t SimdSha256SecondPreimage(
	SimdSha2SecondPreimageContext* Context,
	const size_t Length,
	const uint8_t* Buffers[]);

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
__m256i SimdCalculateS0(
	const __m256i A);
__m256i SimdCalculateS1(
	const __m256i E);
__m256i SimdCalculateTemp1(
	const __m256i E,
	const __m256i F,
	const __m256i G,
	const __m256i H,
	const __m256i K,
	const __m256i W);
__m256i SimdCalculateTemp2(
	const __m256i A,
	const __m256i B,
	const __m256i C);
#endif

#ifdef __cplusplus
}
#endif

#endif /* simdhash_h */
