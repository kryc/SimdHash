//
//  sha2.h
//  SimdHash
//
//  Created by Gareth Evans on 26/08/2020.
//  Copyright © 2020 Gareth Evans. All rights reserved.
//

#ifndef sha2_h
#define sha2_h

#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>	// AVX

#define SIMD_COUNT 8
#define BUFFER_SIZE_DWORDS (64/4)
#define SHA256_SIZE (256/8)

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

typedef union _SimdShaValue
{
	uint32_t u32[256/32];
	__m256i  u256;
} SimdShaValue;

typedef struct _SimdSha2Context
{
	SimdShaValue H[8];
	SimdShaValue Buffer[BUFFER_SIZE_DWORDS];
	uint64_t     Length;
	uint64_t     BitLength;
	size_t       Lanes;
} SimdSha2Context;

typedef struct _Sha2Context
{
	uint32_t H[8];
	uint32_t Buffer[BUFFER_SIZE_DWORDS];
	uint64_t Length;
	uint64_t BitLength;
} Sha2Context;

typedef struct _SimdSha2SecondPreimageContext
{
	SimdSha2Context ShaContext;
	uint8_t 		Target8[SHA256_SIZE];
	uint32_t 		Target32[SHA256_SIZE/4];
} SimdSha2SecondPreimageContext;

void SimdSha256Init(
	SimdSha2Context* Context,
	const size_t Lanes);

void SimdSha256Update(
	SimdSha2Context* Context,
	const size_t Length,
	const uint8_t* Buffers[]);

void SimdSha256Finalize(
	SimdSha2Context* Context);

void SimdSha256SecondPreimageInit(
	SimdSha2SecondPreimageContext* Context,
	SimdSha2Context* ShaContext,
	const uint8_t* Target);

size_t SimdSha256SecondPreimage(
	SimdSha2SecondPreimageContext* Context,
	const size_t Length,
	const uint8_t* Buffers[]);

void SimdSha256GetHashes(
	SimdSha2Context* Context,
	uint8_t** HashBuffers);

void SimdSha256GetHash(
	SimdSha2Context* Context,
	uint8_t* HashBuffer,
	const size_t Lane);

void Sha256Init(
	Sha2Context* Context);

void Sha256Update(
	Sha2Context* Context,
	const size_t Length,
	const uint8_t* Buffer);

void Sha256Finalize(
	Sha2Context* Context);

#ifdef __cplusplus
}
#endif

#endif /* sha2_h */
