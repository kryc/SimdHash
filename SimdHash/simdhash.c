//
//  simdhash.c
//  SimdHash
//
//  Created by Gareth Evans on 17/01/2024.
//  Copyright © 2024 Gareth Evans. All rights reserved.
//

#include "simdhash.h"
#include "simdcommon.h"

const size_t
SimdLanes(
	void
)
{
	return (SIMD_WIDTH / 32);
}

void
SimdHashGetHash(
	SimdHashContext* Context,
	uint8_t* HashBuffer,
	const size_t Lane
)
{
	uint32_t* nextBuffer;
	
	nextBuffer = (uint32_t*)HashBuffer;

    for (size_t i = 0; i < Context->HSize; i++)
    {
        nextBuffer[i] = Context->H[i].epi32_u32[Lane];
    }
}

void
SimdHashGetHashes2D(
	SimdHashContext* Context,
	uint8_t** HashBuffers
)
{
	for (size_t i = 0; i < Context->Lanes; i++)
	{
		SimdHashGetHash(Context, HashBuffers[i], i);
	}
}

void
SimdHashGetHashes(
	SimdHashContext* Context,
	uint8_t* HashBuffers
)
{
    size_t hashsize = Context->HashSize;

	for (size_t i = 0; i < Context->Lanes; i++)
	{
		SimdHashGetHash(Context, &HashBuffers[(i * hashsize)], i);
	}
}