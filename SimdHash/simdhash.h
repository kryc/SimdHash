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

#include "simdcommon.h"

#define MD5_BUFFER_SIZE (64)
#define MD5_BUFFER_SIZE_DWORDS (MD5_BUFFER_SIZE / 4)
#define MD5_H_COUNT (4)
#define MD5_SIZE (MD5_H_COUNT * 4)
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

typedef enum _HashAlgorithm
{
	HashUnknown,
	HashMd5,
	HashSha1,
	HashSha256
} HashAlgorithm;

typedef union _SimdValue
{
	uint8_t  epi32_u8 [SIMD_WIDTH/32][4];	// Access to each lane as a uint8 array
	uint32_t epi32_u32[SIMD_WIDTH/32];		// Access to each lane as a uint32
	simd_t   usimd;
} SimdValue __attribute__((__aligned__(64)));

typedef struct _SimdHashContext
{
	SimdValue H[MAX_H_COUNT];
	SimdValue Buffer[MAX_BUFFER_SIZE_DWORDS];
	size_t    HSize;
	size_t    HashSize;
	size_t    BufferSize;
	uint64_t  Length[MAX_LANES];
	uint64_t  BitLength[MAX_LANES];
	size_t    Lanes;
	uint32_t  BigEndian;
	HashAlgorithm Algorithm;
} SimdHashContext;

#ifdef __cplusplus
extern "C" {
#endif

const size_t
SimdLanes(
	void
);

const HashAlgorithm
ParseHashAlgorithm(
	const char* AlgorithmString
);

const char*
HashAlgorithmToString(
	const HashAlgorithm
);

const size_t
GetHashWidth(
	const HashAlgorithm
);

void
SimdHashInit(
	SimdHashContext* Context,
	const HashAlgorithm Algorithm
);

void
SimdHashUpdate(
	SimdHashContext* Context,
	const size_t Lengths[],
	const uint8_t* Buffers[]
);

void
SimdHashUpdateAll(
	SimdHashContext* Context,
	const size_t Length,
	const uint8_t* Buffers[]
);

void
SimdHashFinalize(
	SimdHashContext* Context
);

//
// MD5
//
void SimdMd5Init(
	SimdHashContext* Context);

// void SimdMd5Update(
// 	SimdHashContext* Context,
// 	const size_t Length,
// 	const uint8_t* Buffers[]);

void SimdMd5Finalize(
	SimdHashContext* Context);

//
// SHA1
//
void SimdSha1Init(
	SimdHashContext* Context);

// void SimdSha1Update(
// 	SimdHashContext* Context,
// 	const size_t Lengths[],
// 	const uint8_t* Buffers[]);

void SimdSha1Finalize(
	SimdHashContext* Context);

//
// SHA256
//
void SimdSha256Init(
	SimdHashContext* Context);

// void SimdSha256Update(
// 	SimdHashContext* Context,
// 	const size_t Length,
// 	const uint8_t* Buffers[]);

void SimdSha256Finalize(
	SimdHashContext* Context);

//
// SimdHash Utility
//
void
SimdHashGetHash(
	SimdHashContext* Context,
	uint8_t* HashBuffer,
	const size_t Lane);

void
SimdHashGetHashes2D(
	SimdHashContext* Context,
	uint8_t** HashBuffers);

void
SimdHashGetHashes(
	SimdHashContext* Context,
	uint8_t* HashBuffers);

#ifdef TEST
simd_t SimdCalculateS0(
	const simd_t A);
simd_t SimdCalculateS1(
	const simd_t E);
simd_t SimdCalculateExtendS0(
	const simd_t A);
simd_t SimdCalculateExtendS1(
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
