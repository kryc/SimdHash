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
#include "shacommon.h"

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

inline
simd_t
SimdCalculateS1(
	const simd_t E)
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
	const simd_t A)
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
	const simd_t W)
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
	const simd_t W)
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
	const simd_t C)
{
	simd_t s0 = SimdCalculateS0(A);
	simd_t maj = SimdShaBitwiseMajority(A, B, C);
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
	const simd_t W)
{
	simd_t s1 = SimdCalculateS1(E);
	simd_t ch = SimdShaBitwiseChoiceWithControl(F, G, E);
	simd_t ret = add_epi32(H, s1);
	ret = add_epi32(ret, ch);
	ret = add_epi32(ret, K);
	return add_epi32(ret, W);
}

static inline
void
SimdSha256ExpandMessageSchedule(
	SimdShaContext* Context,
	SimdValue* MessageSchedule)
{
	for (size_t i = 0; i < SHA256_BUFFER_SIZE_DWORDS; i++)
	{
		simd_t w = load_simd(&Context->Buffer[i].usimd);
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

extern
void
SimdSha256Init(
	SimdShaContext* Context,
	const size_t Lanes)
{
	for (size_t i = 0; i < 8; i++)
	{
		store_simd(&Context->H[i].usimd, set1_epi32(Sha256InitialValues[i]));
	}
	memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
	Context->HSize = SHA256_H_COUNT;
	Context->BufferSize = SHA256_BUFFER_SIZE;
	Context->Length = 0;
	Context->BitLength = 0;
	assert(Lanes <= SIMD_COUNT);
	Context->Lanes = Lanes;
}

static inline void
SimdSha256Transform(
	SimdShaContext* Context)
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

void
SimdSha256Update(
	SimdShaContext* Context,
	const size_t Length,
	const uint8_t* Buffers[])
{
	size_t toWrite = Length;
	size_t offset;
	
	while (toWrite > 0)
	{
		offset = Length - toWrite;
		toWrite = SimdShaUpdateBuffer(Context, offset, Length, Buffers, 1);

		if (Context->Length == SHA256_BUFFER_SIZE)
		{
			SimdSha256Transform(Context);
			memset(Context->Buffer, 0, sizeof(Context->Buffer));
			Context->Length = 0;
		}
	}
}

static inline
void
SimdSha256AppendSize(
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
		SimdSha256Transform(Context);
		memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
	}
	
	//
	// Write the length into the last 64 bits
	//
	store_simd(&Context->Buffer[14].usimd, set1_epi32(Context->BitLength >> 32));
	store_simd(&Context->Buffer[15].usimd, set1_epi32(Context->BitLength & 0xffffffff));
}

void
SimdSha256Finalize(
	SimdShaContext* Context)
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

void SimdSha256GetHash(
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
	nextBuffer[5] = Context->H[5].epi32_u32[Lane];
	nextBuffer[6] = Context->H[6].epi32_u32[Lane];
	nextBuffer[7] = Context->H[7].epi32_u32[Lane];
}

void SimdSha256GetHashes2D(
	SimdShaContext* Context,
	uint8_t** HashBuffers)
{
	for (size_t i = 0; i < Context->Lanes; i++)
	{
		SimdSha256GetHash(Context, HashBuffers[i], i);
	}
}

void SimdSha256GetHashes(
	SimdShaContext* Context,
	uint8_t* HashBuffers)
{
	for (size_t i = 0; i < Context->Lanes; i++)
	{
		SimdSha256GetHash(Context, &HashBuffers[(i * SHA256_SIZE)], i);
	}
}

#define GET_HASH(pOut, iLane){ \
	pOut[(iLane * SHA256_H_COUNT) + 0] = Context->H[0].epi32_u32[(iLane)]; \
	pOut[(iLane * SHA256_H_COUNT) + 1] = Context->H[1].epi32_u32[(iLane)]; \
	pOut[(iLane * SHA256_H_COUNT) + 2] = Context->H[2].epi32_u32[(iLane)]; \
	pOut[(iLane * SHA256_H_COUNT) + 3] = Context->H[3].epi32_u32[(iLane)]; \
	pOut[(iLane * SHA256_H_COUNT) + 4] = Context->H[4].epi32_u32[(iLane)]; \
	pOut[(iLane * SHA256_H_COUNT) + 5] = Context->H[5].epi32_u32[(iLane)]; \
	pOut[(iLane * SHA256_H_COUNT) + 6] = Context->H[6].epi32_u32[(iLane)]; \
	pOut[(iLane * SHA256_H_COUNT) + 7] = Context->H[7].epi32_u32[(iLane)]; \
}

void SimdSha256GetHashesUnrolled(
	SimdShaContext* Context,
	uint8_t* HashBuffers)
{
	uint32_t* out = (uint32_t*)HashBuffers;

	switch (Context->Lanes)
	{
		case 8:
			GET_HASH(out, 7);
		case 7:
			GET_HASH(out, 6);
		case 6:
			GET_HASH(out, 5);
		case 5:
			GET_HASH(out, 4);
		case 4:
			GET_HASH(out, 3);
		case 3:
			GET_HASH(out, 2);
		case 2:
			GET_HASH(out, 1);
		case 1:
			GET_HASH(out, 0);
	}
}