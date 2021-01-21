//
//  sha1.c
//  SimdHash
//
//  Created by Gareth Evans on 20/01/2021.
//  Copyright © 2021 Gareth Evans. All rights reserved.
//

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>		// memset
#include <immintrin.h>	// AVX
#include "simdhash.h"
#include "shacommon.h"

static const uint32_t Sha1InitialValues[] = {
	0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
};

static const uint32_t Sha1RoundConstants[] = {
	0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
};

void
SimdSha1Init(
	SimdShaContext* Context,
	const size_t Lanes)
{
	for (size_t i = 0; i < 5; i++)
	{
		_mm256_store_si256(&Context->H[i].u256, _mm256_set1_epi32(Sha1InitialValues[i]));
	}
	memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
	Context->HSize = SHA1_H_COUNT;
	Context->BufferSize = SHA1_BUFFER_SIZE;
	Context->Length = 0;
	Context->BitLength = 0;
	assert(Lanes <= SIMD_COUNT);
	Context->Lanes = Lanes;
}

void
SimdSha1Update(
	SimdShaContext* Context,
	const size_t Length,
	const uint8_t* Buffers[])
{
	size_t toWrite = Length;
	size_t offset;
	
	while (toWrite > 0)
	{
		offset = Length - toWrite;
		toWrite = SimdShaUpdateBuffer(Context, offset, Length, Buffers);

		if (Context->Length == SHA256_BUFFER_SIZE)
		{
			//SimdSha1Transform(Context);
			memset(Context->Buffer, 0, sizeof(Context->Buffer));
			Context->Length = 0;
		}
	}
}
