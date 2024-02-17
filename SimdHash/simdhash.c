//
//  simdhash.c
//  SimdHash
//
//  Created by Gareth Evans on 17/01/2024.
//  Copyright © 2024 Gareth Evans. All rights reserved.
//

#include <string.h>

#include "simdhash.h"
#include "simdcommon.h"
#include "hashcommon.h"

const size_t
SimdLanes(
	void
)
{
	return (SIMD_WIDTH / 32);
}

const HashAlgorithm
ParseHashAlgorithm(
	const char* AlgorithmString
)
{
	if (strcmp(AlgorithmString, "md5") == 0 ||
		strcmp(AlgorithmString, "MD5") == 0)
	{
		return HashMd5;
	}
	else if (strcmp(AlgorithmString, "sha1") == 0 ||
		strcmp(AlgorithmString, "SHA1") == 0)
	{
		return HashSha1;
	}
	else if (strcmp(AlgorithmString, "sha256") == 0 ||
		strcmp(AlgorithmString, "SHA256") == 0)
	{
		return HashSha256;
	}
	return HashUnknown;
}

const char*
HashAlgorithmToString(
	const HashAlgorithm Algorithm
)
{
	switch (Algorithm)
	{
	case HashMd5:
		return "MD5";
	case HashSha1:
		return "SHA1";
	case HashSha256:
		return "SHA256";
	default:
		return "Unknown";
	}
}

const size_t
GetHashWidth(
	const HashAlgorithm Algorithm
)
{
	switch (Algorithm)
	{
	case HashMd5:
		return MD5_SIZE;
	case HashSha1:
		return SHA1_SIZE;
	case HashSha256:
		return SHA1_SIZE;
	default:
		return (size_t)-1;
	}
}

void SimdHashInit(
	SimdHashContext* Context,
	const HashAlgorithm Algorithm
)
{
	switch (Algorithm)
	{
	case HashMd5:
		SimdMd5Init(Context);
		break;
	case HashSha1:
		SimdSha1Init(Context);
		break;
	case HashSha256:
		SimdSha256Init(Context);
		break;
	case HashUnknown:
		break;
	}
}

void
SimdHashUpdate(
	SimdHashContext* Context,
	const size_t Lengths[],
	const uint8_t* Buffers[]
)
{
	for (size_t lane = 0; lane < Context->Lanes; lane++)
	{
		size_t toWrite = Lengths[lane];

		//
		// For now, this technique has a maximum length
		// of 55 as this is the maximum number of bytes you
		// can fit in a single buffer without needing to
		// perform a sha1 transform.
		//
		toWrite = toWrite > 55 ? 55 : toWrite;

		toWrite = SimdHashUpdateLaneBuffer(
			Context,
			lane,
			0,
			Lengths[lane],
			Buffers[lane]
		);
	}
}

void
SimdHashUpdateAll(
	SimdHashContext* Context,
	const size_t Length,
	const uint8_t* Buffers[]
)
{
	size_t lengths[MAX_LANES];

	for (size_t lane = 0; lane < Context->Lanes; lane++)
	{
		lengths[lane] = Length;
	}

	return SimdHashUpdate(Context, lengths, Buffers);
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

void
SimdHashFinalize(
	SimdHashContext* Context
)
{
	switch (Context->Algorithm)
	{
	case HashMd5:
		SimdMd5Finalize(Context);
		break;
	case HashSha1:
		SimdSha1Finalize(Context);
		break;
	case HashSha256:
		SimdSha256Finalize(Context);
		break;
	case HashUnknown:
		break;
	}
}