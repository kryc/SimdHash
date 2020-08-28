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

#ifdef _MSC_VER
  // MSVC...
  #define ALIGN(n) declspec(align(n))
#else
  // the civilised world...
  #define ALIGN(n) __attribute__ ((aligned(n)))
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

void SimdSha256Init(SimdSha2Context* Context, size_t Lanes);
void SimdSha256Update(SimdSha2Context* Context, size_t Length, uint8_t* Buffers[]);
int  SimdSha256TransformWithTarget(SimdSha2Context* Context, uint32_t targetHValue);
void SimdSha256Finalize(SimdSha2Context* Context);

#endif /* sha2_h */
