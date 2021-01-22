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

		if (Context->Length == SHA256_BUFFER_SIZE)
		{
			SimdSha1Transform(Context);
			memset(Context->Buffer, 0, sizeof(Context->Buffer));
			Context->Length = 0;
		}
	}
}
