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

static inline void
WriteSimdArrayToLinearBuffer(
	const SimdValue* Array,
	const size_t Count,
	uint8_t* HashBuffers
)
{
#ifdef __AVX512F__
	for (size_t i = 0; i < Count; i++)
	{
		__m512i h = _mm512_load_si512(&Array[i].usimd);
		__m512i index = _mm512_setr_epi32(
			(Count * 0) + i,
			(Count * 1) + i,
			(Count * 2) + i,
			(Count * 3) + i,
			(Count * 4) + i,
			(Count * 5) + i,
			(Count * 6) + i,
			(Count * 7) + i,
			(Count * 8) + i,
			(Count * 9) + i,
			(Count * 10) + i,
			(Count * 11) + i,
			(Count * 12) + i,
			(Count * 13) + i,
			(Count * 14) + i,
			(Count * 15) + i
		);
		_mm512_i32scatter_epi32(HashBuffers, index, h, 4);
	}
#else
	uint32_t* buffer = (uint32_t*)HashBuffers;
	for (size_t l = 0; l < SimdLanes(); l++)
	{
		for (size_t i = 0; i < Count; i++)
		{
			buffer[(l * Count) + i] = Array[i].epi32_u32[l];
		}
	}
#endif
}

void
SimdHashGetHashes(
	SimdHashContext* Context,
	uint8_t* HashBuffers
)
{
	WriteSimdArrayToLinearBuffer(Context->H, Context->HSize, HashBuffers);
}

void
SimdHashExtendEntropyAndGetHashes(
	SimdHashContext* Context,
	uint8_t* HashBuffers,
	size_t Length
)
{
	assert(Length > Context->HSize);
	SimdValue buffer[Length];

	for (size_t i = 0; i < Context->HSize; i++)
	{
		simd_t w = load_simd(&Context->H[i].usimd);
		buffer[i].usimd = w;
	}

	for (size_t i = Context->HSize; i < Length; i++)
	{
		// s0 := (w[i-15] rightrotate  7) xor (w[i-15] rightrotate 18) xor (w[i-15] rightshift  3)
        // s1 := (w[i-2] rightrotate 17) xor (w[i-2] rightrotate 19) xor (w[i-2] rightshift 10)
        // w[i] := w[i-16] + s0 + w[i-7] + s1
		simd_t s0 = xor_simd(xor_simd(rotr_epi32(buffer[i - Context->HSize].usimd, 7), rotr_epi32(buffer[i - Context->HSize].usimd, 18)), srli_epi32(buffer[i - Context->HSize].usimd, 3));
		simd_t s1 = xor_simd(xor_simd(rotr_epi32(buffer[i - 2].usimd, 17), rotr_epi32(buffer[i - 2].usimd, 19)), srli_epi32(buffer[i - 2].usimd, 10));
		buffer[i].usimd = add_epi32(s0, s1);
	}

	// Output to the hash buffers
	WriteSimdArrayToLinearBuffer(buffer, Length, HashBuffers);
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