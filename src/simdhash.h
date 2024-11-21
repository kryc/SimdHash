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
#include <stdbool.h>

#include "simdcommon.h"

#define MD4_BUFFER_SIZE (64)
#define MD4_BUFFER_SIZE_DWORDS (MD4_BUFFER_SIZE / 4)
#define MD4_H_COUNT (4)
#define MD4_SIZE (MD4_H_COUNT * 4)
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

#define MAX_H_COUNT (SHA256_H_COUNT)
#define MAX_HASH_SIZE (MAX_H_COUNT * 4)
#define MAX_BUFFER_SIZE (SHA256_BUFFER_SIZE)
#define MAX_BUFFER_SIZE_DWORDS (MAX_BUFFER_SIZE / 4)

typedef enum _HashAlgorithm
{
    HashAlgorithmUndefined = 0,
	HashAlgorithmMD4,
    HashAlgorithmMD5,
    HashAlgorithmSHA1,
    HashAlgorithmSHA256,
	HashAlgorithmNTLM,
	HashAlgorithmMax = HashAlgorithmNTLM
} HashAlgorithm;

#define VALUE_ALIGN (SIMD_WIDTH/8)

typedef union _SimdValue
{
	uint8_t  epi32_u8 [SIMD_WIDTH/32][4];	// Access to each lane as a uint8 array
	uint16_t epi32_u16[SIMD_WIDTH/32][2];	// Access to each lane as a uint16 array
	uint32_t epi32_u32[SIMD_WIDTH/32];		// Access to each lane as a uint32
	uint64_t epi64_u64[SIMD_WIDTH/64];		// Access to each lane as a uint64
	union
	{
		simd_t usimd;
		uint8_t __padding[VALUE_ALIGN];
	};
	
} SimdValue __attribute__((__aligned__(VALUE_ALIGN)));

typedef struct _SimdHashContext
{
	SimdValue H[MAX_H_COUNT];
	SimdValue Buffer[MAX_BUFFER_SIZE_DWORDS];
	size_t    HSize;
	size_t    HashSize;
	size_t    BufferSize;
	uint64_t  Offset[MAX_LANES];
	uint64_t  BitLength[MAX_LANES];
	size_t    Lanes;
	HashAlgorithm Algorithm;
} SimdHashContext;

#define SimdHashAlgorithmCount 4
static const HashAlgorithm
SimdHashAlgorithms[SimdHashAlgorithmCount] = {
    HashAlgorithmMD4,
	HashAlgorithmMD5,
    HashAlgorithmSHA1,
    HashAlgorithmSHA256
};

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

const HashAlgorithm
DetectHashAlgorithm(
    const size_t HashLength
);

static inline const HashAlgorithm
DetectHashAlgorithmHex(
    const size_t HashLengthHex
)
{
	return DetectHashAlgorithm(HashLengthHex / 2);
}

const size_t
GetHashWidth(
	const HashAlgorithm
);

static inline void
SimdHashSetLanes(
	SimdHashContext* Context,
	const size_t LaneCount
)
{
	Context->Lanes = LaneCount;
}

static inline const size_t
SimdHashGetLanes(
	SimdHashContext* Context
)
{
	return Context->Lanes;
}

static inline const HashAlgorithm
SimdHashGetAlgorithm(
	SimdHashContext* Context
)
{
	return Context->Algorithm;
}

//
// Generic init/update/finalize
//
void
SimdHashInit(
	SimdHashContext* Context,
	const HashAlgorithm Algorithm
);

void
SimdHashUpdate(
	SimdHashContext* Context,
	const size_t Lengths[],
	const uint8_t* Buffers[]);

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
// Generic single hash function
//
void
SimdHash(
	HashAlgorithm Algorithm,
	const size_t Lengths[],
	const uint8_t* Buffers[],
	uint8_t* HashBuffers
);

//
// MD4
//
void SimdMd4Init(
	SimdHashContext* Context);

void SimdMd4Transform(
	SimdHashContext* Context);

void SimdMd4Finalize(
	SimdHashContext* Context);

//
// MD5
//
void SimdMd5Init(
	SimdHashContext* Context);

void SimdMd5Transform(
	SimdHashContext* Context);

void SimdMd5Finalize(
	SimdHashContext* Context);

//
// SHA1
//
void SimdSha1Init(
	SimdHashContext* Context);

void SimdSha1Transform(
	SimdHashContext* Context,
	const bool Finalize);

void SimdSha1Finalize(
	SimdHashContext* Context);

//
// SHA256
//
void SimdSha256Init(
	SimdHashContext* Context);

void SimdSha256Transform(
	SimdHashContext* Context,
	const bool Finalize);

void SimdSha256Finalize(
	SimdHashContext* Context);

//
// NTLM
//
void SimdHashNTLM(
	const size_t Lengths[],
	const uint8_t* Buffers[],
	uint8_t* HashBuffers);

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

void
SimdHashExtendEntropyAndGetHashes(
	SimdHashContext* Context,
	uint8_t* HashBuffers,
	size_t Length);

//
// SimdHash Internal
//
void
CopyContextLane(
	SimdHashContext* Destination,
	const SimdHashContext* Source,
	const size_t Lane);

void
SimdHashUpdateInternal(
	SimdHashContext* Context,
	const size_t Lengths[],
	const uint8_t* Buffers[]);

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
