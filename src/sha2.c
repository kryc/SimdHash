//
//  simd_sha2.c
//  SimdHash
//
//  Created by Gareth Evans on 26/08/2020.
//  Copyright © 2020 Gareth Evans. All rights reserved.
//

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>		// memset
#include <immintrin.h>	// AVX

#include "simdhash.h"
#include "simdcommon.h"
#include "hashcommon.h"
#include "library.h"

static const uint32_t Sha256InitialValues[8] = {
	0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
	0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

static const uint32_t Sha256RoundConstants[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};
typedef struct _SecondPreimageResult
{
	uint64_t LaneMap : 63;
	uint64_t Result  : 1;
} SecondPreimageResult;

#define SECOND_PREIMAGE_CANDIDATE_FOUND     1
#define SECOND_PREIMAGE_CANDIDATE_NOT_FOUND 0

extern
void
SimdSha256Init(
	SimdHashContext* Context
)
{
	for (size_t i = 0; i < 8; i++)
	{
		store_simd(&Context->H[i].usimd, set1_epi32(Sha256InitialValues[i]));
	}
	memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
	Context->HSize = SHA256_H_COUNT;
	Context->HashSize = SHA256_SIZE;
	Context->BufferSize = SHA256_BUFFER_SIZE;
	Context->Lanes = SimdLanes();
	memset(Context->Length, 0, sizeof(Context->Length));
	memset(Context->BitLength, 0, sizeof(Context->BitLength));
	Context->Algorithm = HashSha256;
}

inline
simd_t
SimdCalculateS1(
	const simd_t E
)
{
	simd_t er6 = rotr_epi32(E, 6);
	simd_t er11 = rotr_epi32(E, 11);
	simd_t er25 = rotr_epi32(E, 25);
	simd_t ret = xor_simd(er6, er11);
	return xor_simd(ret, er25);
}

inline
simd_t
SimdCalculateS0(
	const simd_t A
)
{
	simd_t ar2 = rotr_epi32(A, 2);
	simd_t ar13 = rotr_epi32(A, 13);
	simd_t ar22 = rotr_epi32(A, 22);
	simd_t ret = xor_simd(ar2, ar13);
	return xor_simd(ret, ar22);
}

// inline
simd_t
SimdCalculateExtendS0(
	const simd_t W
)
{
	simd_t wr7 = rotr_epi32(W, 7);
	simd_t wr18 = rotr_epi32(W, 18);
	simd_t wr3 = srli_epi32(W, 3);
	simd_t ret = xor_simd(wr7, wr18);
	return xor_simd(ret, wr3);
}

// inline
simd_t
SimdCalculateExtendS1(
	const simd_t W
)
{
	simd_t wr17 = rotr_epi32(W, 17);
	simd_t wr19 = rotr_epi32(W, 19);
	simd_t wr10 = srli_epi32(W, 10);
	simd_t ret = xor_simd(wr17, wr19);
	return xor_simd(ret, wr10);
}

inline
simd_t
SimdCalculateTemp2(
	const simd_t A,
	const simd_t B,
	const simd_t C
)
{
	simd_t s0 = SimdCalculateS0(A);
	simd_t maj = SimdBitwiseMajority(A, B, C);
	return add_epi32(s0, maj);
}

inline
simd_t
SimdCalculateTemp1(
	const simd_t E,
	const simd_t F,
	const simd_t G,
	const simd_t H,
	const simd_t K,
	const simd_t W
)
{
	simd_t s1 = SimdCalculateS1(E);
	simd_t ch = SimdBitwiseChoiceWithControl(F, G, E);
	simd_t ret = add_epi32(H, s1);
	ret = add_epi32(ret, ch);
	ret = add_epi32(ret, K);
	return add_epi32(ret, W);
}

static inline
void
SimdSha256ExpandMessageSchedule(
	SimdHashContext* Context,
	SimdValue* MessageSchedule
)
{
	// Load and change endianness from little endian buffer
	for (size_t i = 0; i < SHA256_BUFFER_SIZE_DWORDS; i++)
	{
		simd_t w = bswap_epi32(load_simd(&Context->Buffer[i].usimd));
		store_simd(&MessageSchedule[i].usimd, w);
	}
	
	for (size_t i = SHA256_BUFFER_SIZE_DWORDS; i < SHA256_MESSAGE_SCHEDULE_SIZE_DWORDS; i++)
	{
		simd_t s0 = SimdCalculateExtendS0(load_simd(&MessageSchedule[i-15].usimd));
		simd_t s1 = SimdCalculateExtendS1(load_simd(&MessageSchedule[i-2].usimd));
		simd_t res = add_epi32(load_simd(&MessageSchedule[i-16].usimd), s0);
		res = add_epi32(res, load_simd(&MessageSchedule[i-7].usimd));
		res = add_epi32(res, s1);
		store_simd(&MessageSchedule[i].usimd, res);
	}
}

static inline void
SimdSha256Transform(
	SimdHashContext* Context
)
{
	//
	// Expand the message schedule
	//
	SimdValue messageSchedule[SHA256_MESSAGE_SCHEDULE_SIZE_DWORDS];
	SimdSha256ExpandMessageSchedule(Context, messageSchedule);
	
	simd_t a = load_simd(&Context->H[0].usimd);
	simd_t b = load_simd(&Context->H[1].usimd);
	simd_t c = load_simd(&Context->H[2].usimd);
	simd_t d = load_simd(&Context->H[3].usimd);
	simd_t e = load_simd(&Context->H[4].usimd);
	simd_t f = load_simd(&Context->H[5].usimd);
	simd_t g = load_simd(&Context->H[6].usimd);
	simd_t h = load_simd(&Context->H[7].usimd);

	//
	// Sha256 compression function
	//
	for (size_t i = 0; i < 64; i++)
	{
		simd_t k = set1_epi32(Sha256RoundConstants[i]);
		simd_t w = load_simd(&messageSchedule[i].usimd);
		simd_t temp1 = SimdCalculateTemp1(e, f, g, h, k, w);
		simd_t temp2 = SimdCalculateTemp2(a, b, c);
		h = g;
		g = f;
		f = e;
		e = add_epi32(d, temp1);
		d = c;
		c = b;
		b = a;
		a = add_epi32(temp1, temp2);
	}
	
	//
	// Output to the hash state values
	//
	store_simd(&Context->H[0].usimd, add_epi32(load_simd(&Context->H[0].usimd), a));
	store_simd(&Context->H[1].usimd, add_epi32(load_simd(&Context->H[1].usimd), b));
	store_simd(&Context->H[2].usimd, add_epi32(load_simd(&Context->H[2].usimd), c));
	store_simd(&Context->H[3].usimd, add_epi32(load_simd(&Context->H[3].usimd), d));
	store_simd(&Context->H[4].usimd, add_epi32(load_simd(&Context->H[4].usimd), e));
	store_simd(&Context->H[5].usimd, add_epi32(load_simd(&Context->H[5].usimd), f));
	store_simd(&Context->H[6].usimd, add_epi32(load_simd(&Context->H[6].usimd), g));
	store_simd(&Context->H[7].usimd, add_epi32(load_simd(&Context->H[7].usimd), h));
}

static inline
void
SimdSha256AppendSize(
	SimdHashContext* Context
)
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
		// 	SimdSha256Transform(Context);
		// 	memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
		// }

		// Bump the used buffer length to add the size to
		// the last 64 bits
		Context->Length[lane] = SHA256_BUFFER_SIZE - sizeof(uint32_t) - sizeof(uint32_t);
		// Change endianness to store in the little endian buffer
		uint64_t bitLength = __builtin_bswap64(Context->BitLength[lane]);
		Context->Length[lane] = SimdHashWriteBuffer32(Context, lane, bitLength & 0xffffffff);
		Context->Length[lane] = SimdHashWriteBuffer32(Context, lane, bitLength >> 32);
	}
}

void
SimdSha256Finalize(
	SimdHashContext* Context
)
{
	//
	// Add the message length
	//
	SimdSha256AppendSize(Context);

	//
	// Compute the final transformation
	//
	SimdSha256Transform(Context);
	
	//
	// Change endianness
	//
	simd_t a = bswap_epi32(load_simd(&Context->H[0].usimd));
	simd_t b = bswap_epi32(load_simd(&Context->H[1].usimd));
	simd_t c = bswap_epi32(load_simd(&Context->H[2].usimd));
	simd_t d = bswap_epi32(load_simd(&Context->H[3].usimd));
	simd_t e = bswap_epi32(load_simd(&Context->H[4].usimd));
	simd_t f = bswap_epi32(load_simd(&Context->H[5].usimd));
	simd_t g = bswap_epi32(load_simd(&Context->H[6].usimd));
	simd_t h = bswap_epi32(load_simd(&Context->H[7].usimd));
	
	store_simd(&Context->H[0].usimd, a);
	store_simd(&Context->H[1].usimd, b);
	store_simd(&Context->H[2].usimd, c);
	store_simd(&Context->H[3].usimd, d);
	store_simd(&Context->H[4].usimd, e);
	store_simd(&Context->H[5].usimd, f);
	store_simd(&Context->H[6].usimd, g);
	store_simd(&Context->H[7].usimd, h);
}
