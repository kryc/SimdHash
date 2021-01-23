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
#include "simdcommon.h"
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

static inline void
SimdSha1ExpandMessageSchedule(
	SimdShaContext* Context,
	SimdValue* MessageSchedule)
{
	for (size_t i = 0; i < SHA1_BUFFER_SIZE_DWORDS; i++)
	{
		__m256i w = _mm256_load_si256(&Context->Buffer[i].u256);
		_mm256_store_si256(&MessageSchedule[i].u256, w);
	}
	
	for (size_t i = SHA1_BUFFER_SIZE_DWORDS; i < SHA1_MESSAGE_SCHEDULE_SIZE_DWORDS; i++)
	{
		// w[i] = (w[i-3] xor w[i-8] xor w[i-14] xor w[i-16]) leftrotate 1
		__m256i w = _mm256_load_si256(&MessageSchedule[i-3].u256);
		w = _mm256_xor_si256(w, _mm256_load_si256(&MessageSchedule[i-8].u256));
		w = _mm256_xor_si256(w, _mm256_load_si256(&MessageSchedule[i-14].u256));
		w = _mm256_xor_si256(w, _mm256_load_si256(&MessageSchedule[i-16].u256));
		w = _mm256_rotl_epi32(w, 1);
		_mm256_store_si256(&MessageSchedule[i].u256, w);
	}
}

static inline void
SimdSha1Transform(
	SimdShaContext* Context)
{
	__m256i f, k;
	//
	// Expand the message schedule
	//
	ALIGN(32) SimdValue messageSchedule[SHA1_MESSAGE_SCHEDULE_SIZE_DWORDS];
	SimdSha1ExpandMessageSchedule(Context, messageSchedule);
	
	__m256i a = _mm256_load_si256(&Context->H[0].u256);
	__m256i b = _mm256_load_si256(&Context->H[1].u256);
	__m256i c = _mm256_load_si256(&Context->H[2].u256);
	__m256i d = _mm256_load_si256(&Context->H[3].u256);
	__m256i e = _mm256_load_si256(&Context->H[4].u256);

	//
	// Sha1 compression function
	//
	for (size_t i = 0; i < 80; i++)
	{
		if (i < 20)
		{
			// f = (b and c) or ((not b) and d)
			f = SimdShaBitwiseChoiceWithControl(c, d, b);
			k = _mm256_set1_epi32(Sha1RoundConstants[0]);
		}
		else if (i < 40)
		{
			// f = b xor c xor d
			f = _mm256_xor_si256(b, _mm256_xor_si256(c, d));
			k = _mm256_set1_epi32(Sha1RoundConstants[1]);
		}
		else if (i < 60)
		{
			// f = (b and c) or (b and d) or (c and d)
			f = SimdShaBitwiseMajority(b, c, d);
			k = _mm256_set1_epi32(Sha1RoundConstants[2]);
		}
		else //if (i < 80)
		{
			// f = b xor c xor d
			f = _mm256_xor_si256(b, _mm256_xor_si256(c, d));
			k = _mm256_set1_epi32(Sha1RoundConstants[3]);
		}
		
		__m256i w = _mm256_load_si256(&messageSchedule[i].u256);
		__m256i temp = _mm256_rotl_epi32(a, 5);
		temp = _mm256_add_epi32(temp, f);
		temp = _mm256_add_epi32(temp, e);
		temp = _mm256_add_epi32(temp, k);
		temp = _mm256_add_epi32(temp, w);
		e = d;
		d = c;
		c = _mm256_rotl_epi32(b, 30);
		b = a;
		a = temp;
	}
	
	//
	// Output to the hash state values
	//
	_mm256_store_si256(&Context->H[0].u256, _mm256_add_epi32(_mm256_load_si256(&Context->H[0].u256), a));
	_mm256_store_si256(&Context->H[1].u256, _mm256_add_epi32(_mm256_load_si256(&Context->H[1].u256), b));
	_mm256_store_si256(&Context->H[2].u256, _mm256_add_epi32(_mm256_load_si256(&Context->H[2].u256), c));
	_mm256_store_si256(&Context->H[3].u256, _mm256_add_epi32(_mm256_load_si256(&Context->H[3].u256), d));
	_mm256_store_si256(&Context->H[4].u256, _mm256_add_epi32(_mm256_load_si256(&Context->H[4].u256), e));
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

		if (Context->Length == SHA1_BUFFER_SIZE)
		{
			SimdSha1Transform(Context);
			memset(Context->Buffer, 0, sizeof(Context->Buffer));
			Context->Length = 0;
		}
	}
}

static inline
void
SimdSha1AppendSize(
	SimdShaContext* Context)
/*++
 Appends the 1-bit and the message length to the hash buffer
 Also performs the additional Transform step if required
 --*/
{
	//
	// Write the 1 bit
	//
	size_t bufferIndex = Context->Length / 4;
	size_t bufferOffset = Context->Length % 4;
	
	for (size_t lane = 0; lane < Context->Lanes; lane++)
	{
		Context->Buffer[bufferIndex].epi32_u8[lane][(sizeof(uint32_t) - 1 - bufferOffset)] = 0x80;
	}
	Context->Length++;
	
	if (Context->Length >= 56)
	{
		SimdSha1Transform(Context);
		memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
	}
	
	//
	// Write the length into the last 64 bits
	//
	_mm256_store_si256(&Context->Buffer[14].u256, _mm256_set1_epi32(Context->BitLength >> 32));
	_mm256_store_si256(&Context->Buffer[15].u256, _mm256_set1_epi32(Context->BitLength & 0xffffffff));
}

void
SimdSha1Finalize(
	SimdShaContext* Context)
{
	//
	// Add the message length
	//
	SimdSha1AppendSize(Context);

	//
	// Compute the final transformation
	//
	SimdSha1Transform(Context);
	
	//
	// Change endianness
	//
	__m256i shufMask = _mm256_setr_epi8(3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12,19,18,17,16,23,22,21,20,27,26,25,24,31,30,29,28);
	__m256i a = _mm256_shuffle_epi8(_mm256_load_si256(&Context->H[0].u256), shufMask);
	__m256i b = _mm256_shuffle_epi8(_mm256_load_si256(&Context->H[1].u256), shufMask);
	__m256i c = _mm256_shuffle_epi8(_mm256_load_si256(&Context->H[2].u256), shufMask);
	__m256i d = _mm256_shuffle_epi8(_mm256_load_si256(&Context->H[3].u256), shufMask);
	__m256i e = _mm256_shuffle_epi8(_mm256_load_si256(&Context->H[4].u256), shufMask);
	
	_mm256_store_si256(&Context->H[0].u256, a);
	_mm256_store_si256(&Context->H[1].u256, b);
	_mm256_store_si256(&Context->H[2].u256, c);
	_mm256_store_si256(&Context->H[3].u256, d);
	_mm256_store_si256(&Context->H[4].u256, e);
}

void SimdSha1GetHash(
	SimdShaContext* Context,
	uint8_t* HashBuffer,
	const size_t Lane)
{
	uint32_t* nextBuffer;
	
	nextBuffer = (uint32_t*)HashBuffer;
	nextBuffer[0] = Context->H[0].epi32_u32[Lane];
	nextBuffer[1] = Context->H[1].epi32_u32[Lane];
	nextBuffer[2] = Context->H[2].epi32_u32[Lane];
	nextBuffer[3] = Context->H[3].epi32_u32[Lane];
	nextBuffer[4] = Context->H[4].epi32_u32[Lane];
}

void SimdSha1GetHashes(
	SimdShaContext* Context,
	uint8_t** HashBuffers)
{
	for (size_t i = 0; i < Context->Lanes; i++)
	{
		SimdSha1GetHash(Context, HashBuffers[i], i);
	}
}
