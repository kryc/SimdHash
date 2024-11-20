//
//  simdhash.c
//  SimdHash
//
//  Created by Gareth Evans on 17/01/2024.
//  Copyright © 2024 Gareth Evans. All rights reserved.
//

#include <string.h>
#include <unicode/ucnv.h>
#include <unicode/ustring.h>

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
	if (strcmp(AlgorithmString, "md4") == 0 ||
		strcmp(AlgorithmString, "MD4") == 0)
	{
		return HashAlgorithmMD4;
	}
	else if (strcmp(AlgorithmString, "md5") == 0 ||
		strcmp(AlgorithmString, "MD5") == 0)
	{
		return HashAlgorithmMD5;
	}
	else if (strcmp(AlgorithmString, "sha1") == 0 ||
		strcmp(AlgorithmString, "SHA1") == 0)
	{
		return HashAlgorithmSHA1;
	}
	else if (strcmp(AlgorithmString, "sha256") == 0 ||
		strcmp(AlgorithmString, "SHA256") == 0)
	{
		return HashAlgorithmSHA256;
	}
	else if (strcmp(AlgorithmString, "ntlm") == 0 ||
		strcmp(AlgorithmString, "NTLM") == 0)
	{
		return HashAlgorithmNTLM;
	}
	return HashAlgorithmUndefined;
}

const char*
HashAlgorithmToString(
	const HashAlgorithm Algorithm
)
{
	switch (Algorithm)
	{
	case HashAlgorithmMD4:
		return "MD4";
	case HashAlgorithmMD5:
		return "MD5";
	case HashAlgorithmSHA1:
		return "SHA1";
	case HashAlgorithmSHA256:
		return "SHA256";
	case HashAlgorithmNTLM:
		return "NTLM";
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
	case HashAlgorithmMD4:
	case HashAlgorithmNTLM:
		return MD4_SIZE;
	case HashAlgorithmMD5:
		return MD5_SIZE;
	case HashAlgorithmSHA1:
		return SHA1_SIZE;
	case HashAlgorithmSHA256:
		return SHA1_SIZE;
	default:
		return (size_t)-1;
	}
}

const HashAlgorithm
DetectHashAlgorithm(
    const size_t HashLength
)
{
	switch(HashLength)
	{
	case MD5_SIZE:
		return HashAlgorithmMD5;
	case SHA1_SIZE:
		return HashAlgorithmSHA1;
	case SHA256_SIZE:
		return HashAlgorithmSHA256;
	default:
		return HashAlgorithmUndefined;
	}
}

void SimdHashInit(
	SimdHashContext* Context,
	const HashAlgorithm Algorithm
)
{
	switch (Algorithm)
	{
	case HashAlgorithmMD4:
		SimdMd4Init(Context);
		break;
	case HashAlgorithmMD5:
		SimdMd5Init(Context);
		break;
	case HashAlgorithmSHA1:
		SimdSha1Init(Context);
		break;
	case HashAlgorithmSHA256:
		SimdSha256Init(Context);
		break;
	case HashAlgorithmUndefined:
		break;
	case HashAlgorithmNTLM:
		SimdMd4Init(Context);
		Context->Algorithm = HashAlgorithmNTLM;
		break;
	}
}

void
SimdHashSetLaneCount(
	SimdHashContext* Context,
	const size_t LaneCount
)
{
	Context->Lanes = LaneCount;
}

static void
SimdHashUpdateInternal(
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
			Lengths[lane],
			Buffers[lane]
		);
	}
}

static void
SimdHashUpdateNTLM(
	SimdHashContext* Context,
	const size_t Lengths[],
	const uint8_t* Buffers[]
)
{
	size_t newLengths[MAX_LANES];
	uint8_t* newBuffers[MAX_LANES];

	UErrorCode status = U_ZERO_ERROR;

	for (size_t i = 0; i < Context->Lanes; i++)
	{
		int32_t newLength;
		u_strFromUTF8Lenient(NULL, 0, &newLength, (const char*)Buffers[i], Lengths[i], &status);
		if (status != U_BUFFER_OVERFLOW_ERROR && status != U_STRING_NOT_TERMINATED_WARNING)
		{ 
			fprintf(stderr, "Error: %s\n", u_errorName(status));
			// Fallback to hash the provided input
			newBuffers[i] = (uint8_t*)Buffers[i];
			newLengths[i] = Lengths[i];
			continue;
		}

		// Reset the status and allocate memory
		status = U_ZERO_ERROR;
		newBuffers[i] = (uint8_t*)alloca((newLength + 1) * sizeof(UChar));
		if (newBuffers[i] == NULL) {
			fprintf(stderr, "Memory allocation error\n");
			// Fallback to hash the provided input
			newBuffers[i] = (uint8_t*)Buffers[i];
			newLengths[i] = Lengths[i];
			continue;
		}

		// Convert UTF-8 to UTF-16
		u_strFromUTF8Lenient((UChar*)newBuffers[i], newLength + 1, NULL, (const char*)Buffers[i], Lengths[i], &status);
		if (U_FAILURE(status)) {
			fprintf(stderr, "Conversion error: %s\n", u_errorName(status));
			// Fallback to hash the provided input
			newBuffers[i] = (uint8_t*)Buffers[i];
			newLengths[i] = Lengths[i];
			continue;
		}

		newLengths[i] = (size_t)newLength * sizeof(UChar);
	}

	// Call the internal SimdHash update function
	SimdHashUpdateInternal(
		Context,
		newLengths,
		(const uint8_t**)newBuffers
	);
}

void
SimdHashUpdate(
	SimdHashContext* Context,
	const size_t Lengths[],
	const uint8_t* Buffers[]
)
{
	if (Context->Algorithm != HashAlgorithmNTLM)
	{
		SimdHashUpdateInternal(
			Context,
			Lengths,
			Buffers
		);
	}
	else
	{
		SimdHashUpdateNTLM(
			Context,
			Lengths,
			Buffers
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
	const size_t CountDwords,
	uint8_t* HashBuffers
)
{
#if defined __AVX512F__ && defined AVXSCATTER
	for (size_t i = 0; i < CountDwords; i++)
	{
		__m512i h = _mm512_load_si512(&Array[i].usimd);
		__m512i index = _mm512_setr_epi32(
			(CountDwords * 0) + i,
			(CountDwords * 1) + i,
			(CountDwords * 2) + i,
			(CountDwords * 3) + i,
			(CountDwords * 4) + i,
			(CountDwords * 5) + i,
			(CountDwords * 6) + i,
			(CountDwords * 7) + i,
			(CountDwords * 8) + i,
			(CountDwords * 9) + i,
			(CountDwords * 10) + i,
			(CountDwords * 11) + i,
			(CountDwords * 12) + i,
			(CountDwords * 13) + i,
			(CountDwords * 14) + i,
			(CountDwords * 15) + i
		);
		_mm512_i32scatter_epi32(HashBuffers, index, h, 4);
	}
#else
	uint32_t* buffer = (uint32_t*)HashBuffers;
	for (size_t i = 0; i < CountDwords; i++)
	{
		for (size_t l = 0; l < SimdLanes(); l++)
		{
			buffer[(l * CountDwords) + i] = Array[i].epi32_u32[l];
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
	size_t CountDwords
)
{
	assert(CountDwords > Context->HSize);
	SimdValue buffer[CountDwords];

	for (size_t i = 0; i < Context->HSize; i++)
	{
		buffer[i].usimd = load_simd(&Context->H[i].usimd);
	}

	for (size_t i = Context->HSize; i < CountDwords; i++)
	{
		// s0 := (w[i-15] rightrotate  7) xor (w[i-15] rightrotate 18) xor (w[i-15] rightshift  3)
        // s1 := (w[i-2] rightrotate 17) xor (w[i-2] rightrotate 19) xor (w[i-2] rightshift 10)
        // w[i] := w[i-16] + s0 + w[i-7] + s1
		simd_t s0 = xor_simd(xor_simd(rotr_epi32(buffer[i - Context->HSize].usimd, 7), rotr_epi32(buffer[i - Context->HSize].usimd, 18)), srli_epi32(buffer[i - Context->HSize].usimd, 3));
		simd_t s1 = xor_simd(xor_simd(rotr_epi32(buffer[i - 2].usimd, 17), rotr_epi32(buffer[i - 2].usimd, 19)), srli_epi32(buffer[i - 2].usimd, 10));
		buffer[i].usimd = add_epi32(s0, s1);
	}

	// Output to the hash buffers
	WriteSimdArrayToLinearBuffer(buffer, CountDwords, HashBuffers);
}

void
SimdHashFinalize(
	SimdHashContext* Context
)
{
	switch (Context->Algorithm)
	{
	case HashAlgorithmMD4:
	case HashAlgorithmNTLM:
		SimdMd4Finalize(Context);
		break;
	case HashAlgorithmMD5:
		SimdMd5Finalize(Context);
		break;
	case HashAlgorithmSHA1:
		SimdSha1Finalize(Context);
		break;
	case HashAlgorithmSHA256:
		SimdSha256Finalize(Context);
		break;
	case HashAlgorithmUndefined:
		break;
	}
}

void
SimdHash(
	HashAlgorithm Algorithm,
	const size_t Lengths[],
	const uint8_t* Buffers[],
	uint8_t* HashBuffers
)
{
	SimdHashContext ctx;
	SimdHashInit(&ctx, Algorithm);
	SimdHashUpdate(&ctx, Lengths, Buffers);
	SimdHashFinalize(&ctx);
	SimdHashGetHashes(&ctx, HashBuffers);
}