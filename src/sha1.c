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
	memset(Context->Offset, 0, sizeof(Context->Offset));
	memset(Context->BitLength, 0, sizeof(Context->BitLength));
	Context->Algorithm = HashAlgorithmSHA1;
}

static inline void
SimdSha1Transform(
	SimdHashContext* Context,
	const int Finalize
)
{
	simd_t f, k;
	//
	// Expand the message schedule
	//
	simd_t messageSchedule[SHA1_MESSAGE_SCHEDULE_SIZE_DWORDS];
	// Load and change endianness from little endian buffer
	for (size_t i = 0; i < SHA1_BUFFER_SIZE_DWORDS; i++)
	{
		messageSchedule[i] = bswap_epi32(load_simd(&Context->Buffer[i].usimd));
	}
	
	for (size_t i = SHA1_BUFFER_SIZE_DWORDS; i < SHA1_MESSAGE_SCHEDULE_SIZE_DWORDS; i++)
	{
		// w[i] = (w[i-3] xor w[i-8] xor w[i-14] xor w[i-16]) leftrotate 1
		simd_t w = messageSchedule[i-3];
		w = xor_simd(w, messageSchedule[i-8]);
		w = xor_simd(w, messageSchedule[i-14]);
		w = xor_simd(w, messageSchedule[i-16]);
		messageSchedule[i] = rotl_epi32(w, 1);
	}
	
	simd_t aO, bO, cO, dO, eO;
	simd_t a = aO = load_simd(&Context->H[0].usimd);
	simd_t b = bO = load_simd(&Context->H[1].usimd);
	simd_t c = cO = load_simd(&Context->H[2].usimd);
	simd_t d = dO = load_simd(&Context->H[3].usimd);
	simd_t e = eO = load_simd(&Context->H[4].usimd);

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
		
		simd_t w = messageSchedule[i];
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
	// If finalizing, swap the endianness
	//
	if (Finalize)
	{
		store_simd(&Context->H[0].usimd, bswap_epi32(add_epi32(aO, a)));
		store_simd(&Context->H[1].usimd, bswap_epi32(add_epi32(bO, b)));
		store_simd(&Context->H[2].usimd, bswap_epi32(add_epi32(cO, c)));
		store_simd(&Context->H[3].usimd, bswap_epi32(add_epi32(dO, d)));
		store_simd(&Context->H[4].usimd, bswap_epi32(add_epi32(eO, e)));
	}
	else
	{
		store_simd(&Context->H[0].usimd, add_epi32(aO, a));
		store_simd(&Context->H[1].usimd, add_epi32(bO, b));
		store_simd(&Context->H[2].usimd, add_epi32(cO, c));
		store_simd(&Context->H[3].usimd, add_epi32(dO, d));
		store_simd(&Context->H[4].usimd, add_epi32(eO, e));
	}
	
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
		const size_t offset = Context->Offset[lane];
		SimdHashWriteBuffer8(Context, offset, lane, 0x80);
		
		//
		// Check if we need to do another round
		//
		// if (Context->Offset >= 56)
		// {
		// 	SimdSha1Transform(Context);
		// 	memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
		// }

		// Bump the used buffer length to add the size to
		// the last 64 bits
		SimdHashWriteBuffer64(
			Context,
			SHA1_BUFFER_SIZE - sizeof(uint64_t),
			lane,
			__builtin_bswap64(Context->BitLength[lane]) // Change endianness to store in the little endian buffer
		);
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
	SimdSha1Transform(Context, 1);
}
