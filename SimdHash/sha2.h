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

#endif /* sha2_h */
