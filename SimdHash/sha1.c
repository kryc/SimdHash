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
#include <string.h>
#include <immintrin.h>

#include "simdhash.h"
#include "simdcommon.h"
#include "hashcommon.h"
#include "library.h"

static const uint32_t Sha1InitialValues[] = {
	0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
};

static const uint32_t Sha1RoundConstants[] = {
	0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
};

void
SimdSha1Init(
	SimdHashContext* Context)
{
	for (size_t i = 0; i < 5; i++)
	{
		store_simd(&Context->H[i].usimd, set1_epi32(Sha1InitialValues[i]));
	}
	memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
	Context->HSize = SHA1_H_COUNT;
	Context->HashSize = SHA1_SIZE;
	Context->BufferSize = SHA1_BUFFER_SIZE;
	Context->Lanes = SimdLanes();
	memset(Context->Length, 0, sizeof(Context->Length));
	memset(Context->BitLength, 0, sizeof(Context->BitLength));
	Context->BigEndian = 1;
}

static inline void
SimdSha1ExpandMessageSchedule(
	SimdHashContext* Context,
	SimdValue* MessageSchedule)
{
	for (size_t i = 0; i < SHA1_BUFFER_SIZE_DWORDS; i++)
	{
		simd_t w = load_simd(&Context->Buffer[i].usimd);
		store_simd(&MessageSchedule[i].usimd, w);
	}
	
	for (size_t i = SHA1_BUFFER_SIZE_DWORDS; i < SHA1_MESSAGE_SCHEDULE_SIZE_DWORDS; i++)
	{
		// w[i] = (w[i-3] xor w[i-8] xor w[i-14] xor w[i-16]) leftrotate 1
		simd_t w = load_simd(&MessageSchedule[i-3].usimd);
		w = xor_simd(w, load_simd(&MessageSchedule[i-8].usimd));
		w = xor_simd(w, load_simd(&MessageSchedule[i-14].usimd));
		w = xor_simd(w, load_simd(&MessageSchedule[i-16].usimd));
		w = rotl_epi32(w, 1);
		store_simd(&MessageSchedule[i].usimd, w);
	}
}

static inline void
SimdSha1Transform(
	SimdHashContext* Context)
{
	simd_t f, k;
	//
	// Expand the message schedule
	//
	SimdValue messageSchedule[SHA1_MESSAGE_SCHEDULE_SIZE_DWORDS];
	SimdSha1ExpandMessageSchedule(Context, messageSchedule);
	
	simd_t a = load_simd(&Context->H[0].usimd);
	simd_t b = load_simd(&Context->H[1].usimd);
	simd_t c = load_simd(&Context->H[2].usimd);
	simd_t d = load_simd(&Context->H[3].usimd);
	simd_t e = load_simd(&Context->H[4].usimd);

	//
	// Sha1 compression function
	//
	for (size_t i = 0; i < 80; i++)
	{
		if (i < 20)
		{
			// f = (b and c) or ((not b) and d)
			f = SimdBitwiseChoiceWithControl(c, d, b);
			k = set1_epi32(Sha1RoundConstants[0]);
		}
		else if (i < 40)
		{
			// f = b xor c xor d
			f = xor_simd(b, xor_simd(c, d));
			k = set1_epi32(Sha1RoundConstants[1]);
		}
		else if (i < 60)
		{
			// f = (b and c) or (b and d) or (c and d)
			f = SimdBitwiseMajority(b, c, d);
			k = set1_epi32(Sha1RoundConstants[2]);
		}
		else //if (i < 80)
		{
			// f = b xor c xor d
			f = xor_simd(b, xor_simd(c, d));
			k = set1_epi32(Sha1RoundConstants[3]);
		}
		
		simd_t w = load_simd(&messageSchedule[i].usimd);
		simd_t temp = rotl_epi32(a, 5);
		temp = add_epi32(temp, f);
		temp = add_epi32(temp, e);
		temp = add_epi32(temp, k);
		temp = add_epi32(temp, w);
		e = d;
		d = c;
		c = rotl_epi32(b, 30);
		b = a;
		a = temp;
	}
	
	//
	// Output to the hash state values
	//
	store_simd(&Context->H[0].usimd, add_epi32(load_simd(&Context->H[0].usimd), a));
	store_simd(&Context->H[1].usimd, add_epi32(load_simd(&Context->H[1].usimd), b));
	store_simd(&Context->H[2].usimd, add_epi32(load_simd(&Context->H[2].usimd), c));
	store_simd(&Context->H[3].usimd, add_epi32(load_simd(&Context->H[3].usimd), d));
	store_simd(&Context->H[4].usimd, add_epi32(load_simd(&Context->H[4].usimd), e));
}

static inline
void
SimdSha1AppendSize(
	SimdHashContext* Context)
/*++
 Appends the 1-bit and the message length to the hash buffer
 Also performs the additional Transform step if required
 --*/
{
	for (size_t lane = 0; lane < Context->Lanes; lane++)
	{
		//
		// Write the 1 bit
		//		
		Context->Length[lane] = SimdHashWriteBuffer8(Context, lane, 0x80);
		
		//
		// Check if we need to do another round
		//
		// if (Context->Length >= 56)
		// {
		// 	SimdSha1Transform(Context);
		// 	memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
		// }

		// Bump the used buffer length to add the size to
		// the last 64 bits
		Context->Length[lane] = SHA1_BUFFER_SIZE - sizeof(uint32_t) - sizeof(uint32_t);
		Context->Length[lane] = SimdHashWriteBuffer32NoEndian(Context, lane, Context->BitLength[lane] >> 32);
		Context->Length[lane] = SimdHashWriteBuffer32NoEndian(Context, lane, Context->BitLength[lane] & 0xffffffff);
	}
}

void
SimdSha1Finalize(
	SimdHashContext* Context)
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
	simd_t a = bswap_epi32(load_simd(&Context->H[0].usimd));
	simd_t b = bswap_epi32(load_simd(&Context->H[1].usimd));
	simd_t c = bswap_epi32(load_simd(&Context->H[2].usimd));
	simd_t d = bswap_epi32(load_simd(&Context->H[3].usimd));
	simd_t e = bswap_epi32(load_simd(&Context->H[4].usimd));
	
	store_simd(&Context->H[0].usimd, a);
	store_simd(&Context->H[1].usimd, b);
	store_simd(&Context->H[2].usimd, c);
	store_simd(&Context->H[3].usimd, d);
	store_simd(&Context->H[4].usimd, e);
}
