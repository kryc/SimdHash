//
//  sha2.c
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
#include "sha2.h"

static const uint32_t InitialValues[] = {
	0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
	0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

static const uint32_t RoundConstants[] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static inline
__m256i
RotateLeft32(__m256i Value, uint8_t Distance)
{
	assert(Distance < 32);
	__m256i shl = _mm256_slli_epi32(Value, Distance);
	__m256i shr = _mm256_srli_epi32(Value, 32 - Distance);
	return _mm256_or_si256(shl, shr);
}

static inline
__m256i
RotateRight32(__m256i Value, uint8_t Distance)
{
	assert(Distance < 32);
	__m256i shr = _mm256_srli_epi32(Value, Distance);
	__m256i shl = _mm256_slli_epi32(Value, 32 - Distance);
	return _mm256_or_si256(shl, shr);
}

static inline
__m256i
CalculateS1(__m256i E)
{
	__m256i er6 = RotateRight32(E, 6);
	__m256i er11 = RotateRight32(E, 11);
	__m256i er25 = RotateRight32(E, 25);
	__m256i ret = _mm256_xor_si256(er6, er11);
	return _mm256_xor_si256(ret, er25);
}

static inline
__m256i
CalculateS0(__m256i A)
{
	__m256i ar2 = RotateRight32(A, 2);
	__m256i ar13 = RotateRight32(A, 13);
	__m256i ar22 = RotateRight32(A, 22);
	__m256i ret = _mm256_xor_si256(ar2, ar13);
	return _mm256_xor_si256(ret, ar22);
}

static inline
__m256i
CalculateExtendS0(__m256i W)
{
	__m256i wr7 = RotateRight32(W, 7);
	__m256i wr18 = RotateRight32(W, 18);
	__m256i wr3 = _mm256_srli_epi32(W, 3);
	__m256i ret = _mm256_xor_si256(wr7, wr18);
	return _mm256_xor_si256(ret, wr3);
}

static inline
__m256i
CalculateExtendS1(__m256i W)
{
	__m256i wr17 = RotateRight32(W, 17);
	__m256i wr19 = RotateRight32(W, 19);
	__m256i wr10 = _mm256_srli_epi32(W, 10);
	__m256i ret = _mm256_xor_si256(wr17, wr19);
	return _mm256_xor_si256(ret, wr10);
}

static inline
__m256i
CalculateCh(__m256i E, __m256i F, __m256i G)
{
	__m256i eAndF = _mm256_and_si256(E, F);
	__m256i notEAndG = _mm256_andnot_si256(E, G);
	return _mm256_xor_si256(eAndF, notEAndG);
}

static inline
__m256i
CalculateMaj(__m256i A, __m256i B, __m256i C)
{
	__m256i aAndB = _mm256_and_si256(A, B);
	__m256i aAndC = _mm256_and_si256(A, C);
	__m256i bAndC = _mm256_and_si256(B, C);
	__m256i ret = _mm256_xor_si256(aAndB, aAndC);
	return _mm256_xor_si256(ret, bAndC);
}

static inline
__m256i
CalculateTemp2(__m256i A, __m256i B, __m256i C)
{
	__m256i s0 = CalculateS0(A);
	__m256i maj = CalculateMaj(A, B, C);
	return _mm256_add_epi32(s0, maj);
}

static inline
__m256i
CalculateTemp1(__m256i E, __m256i F, __m256i G, __m256i H, __m256i K, __m256i W)
{
	__m256i s1 = CalculateS1(E);
	__m256i ch = CalculateCh(E, F, G);
	__m256i ret = _mm256_add_epi32(H, s1);
	ret = _mm256_add_epi32(ret, ch);
	ret = _mm256_add_epi32(ret, K);
	return _mm256_add_epi32(ret, W);
}

static inline
void
ExpandMessageSchedule(SimdSha2Context* Context, SimdShaValue* messageSchedule)
{
	for (size_t i = 0; i < 16; i++)
	{
		__m256i w = _mm256_load_si256(&Context->Buffer[i].u256);
		_mm256_store_si256(&messageSchedule[i].u256, w);
	}
	
	for (size_t i = 16; i < 64; i++)
	{
		__m256i s0 = CalculateExtendS0(_mm256_load_si256(&messageSchedule[i-15].u256));
		__m256i s1 = CalculateExtendS1(_mm256_load_si256(&messageSchedule[i-2].u256));
		__m256i res = _mm256_add_epi32(_mm256_load_si256(&messageSchedule[i-16].u256), s0);
		res = _mm256_add_epi32(res, _mm256_load_si256(&messageSchedule[i-7].u256));
		res = _mm256_add_epi32(res, s1);
		_mm256_store_si256(&messageSchedule[i].u256, res);
	}
}

void
SimdSha256Init(SimdSha2Context* Context, size_t Lanes)
{
	for (size_t i = 0; i < 8; i++)
	{
		_mm256_store_si256(&Context->H[i].u256, _mm256_set1_epi32(InitialValues[i]));
	}
	memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
	Context->Length = 0;
	Context->BitLength = 0;
	assert(Lanes <= SIMD_COUNT);
	Context->Lanes = Lanes;
}

void
SimdSha256Transform(SimdSha2Context* Context)
{
	//
	// Expand the message schedule
	//
	ALIGN(16) SimdShaValue messageSchedule[64];
	ExpandMessageSchedule(Context, messageSchedule);
	
	__m256i a = _mm256_load_si256(&Context->H[0].u256), initialA = a;
	__m256i b = _mm256_load_si256(&Context->H[1].u256), initialB = b;
	__m256i c = _mm256_load_si256(&Context->H[2].u256), initialC = c;
	__m256i d = _mm256_load_si256(&Context->H[3].u256), initialD = d;
	__m256i e = _mm256_load_si256(&Context->H[4].u256), initialE = e;
	__m256i f = _mm256_load_si256(&Context->H[5].u256), initialF = f;
	__m256i g = _mm256_load_si256(&Context->H[6].u256), initialG = g;
	__m256i h = _mm256_load_si256(&Context->H[7].u256), initialH = h;

	for (size_t i = 0; i < 64; i++)
	{
		__m256i k = _mm256_set1_epi32(RoundConstants[i]);
		__m256i w = _mm256_load_si256(&messageSchedule[i].u256);
		__m256i temp1 = CalculateTemp1(e, f, g, h, k, w);
		__m256i temp2 = CalculateTemp2(a, b, c);
		h = g;
		g = f;
		f = e;
		e = _mm256_add_epi32(d, temp1);
		d = c;
		c = b;
		b = a;
		a = _mm256_add_epi32(temp1, temp2);
	}
	
	//
	// Output to the hash state values
	//
	_mm256_store_si256(&Context->H[0].u256, _mm256_add_epi32(initialA, a));
	_mm256_store_si256(&Context->H[1].u256, _mm256_add_epi32(initialB, b));
	_mm256_store_si256(&Context->H[2].u256, _mm256_add_epi32(initialC, c));
	_mm256_store_si256(&Context->H[3].u256, _mm256_add_epi32(initialD, d));
	_mm256_store_si256(&Context->H[4].u256, _mm256_add_epi32(initialE, e));
	_mm256_store_si256(&Context->H[5].u256, _mm256_add_epi32(initialF, f));
	_mm256_store_si256(&Context->H[6].u256, _mm256_add_epi32(initialG, g));
	_mm256_store_si256(&Context->H[7].u256, _mm256_add_epi32(initialH, h));
}

int
SimdSha256TransformWithTarget(SimdSha2Context* Context, uint32_t targetHValue)
/*++
	Perform a hash transformation but bail early if the final DWORD
	does not match the target H value. Used for computing collisions
 --*/
{
	//
	// Expand the message schedule
	//
	ALIGN(16) SimdShaValue messageSchedule[64];
	ExpandMessageSchedule(Context, messageSchedule);
	
	__m256i a = _mm256_load_si256(&Context->H[0].u256), initialA = a;
	__m256i b = _mm256_load_si256(&Context->H[1].u256), initialB = b;
	__m256i c = _mm256_load_si256(&Context->H[2].u256), initialC = c;
	__m256i d = _mm256_load_si256(&Context->H[3].u256), initialD = d;
	__m256i e = _mm256_load_si256(&Context->H[4].u256), initialE = e;
	__m256i f = _mm256_load_si256(&Context->H[5].u256), initialF = f;
	__m256i g = _mm256_load_si256(&Context->H[6].u256), initialG = g;
	__m256i h = _mm256_load_si256(&Context->H[7].u256), initialH = h;
	
	__m256i targetH = _mm256_sub_epi32(_mm256_set1_epi32(targetHValue), h);
	
	for (size_t i = 0; i < 64; i++)
	{
		__m256i k = _mm256_set1_epi32(RoundConstants[i]);
		__m256i w = _mm256_load_si256(&messageSchedule[i].u256);
		__m256i temp1 = CalculateTemp1(e, f, g, h, k, w);
		__m256i temp2 = CalculateTemp2(a, b, c);
		h = g;
		g = f;
		f = e;
		e = _mm256_add_epi32(d, temp1);
		
		if (i == 60)
		{
			__m256i cmp = _mm256_cmpeq_epi32(targetH, e);
			int comparison = _mm256_movemask_ps(cmp);
			if (comparison == 0)
				return 1;
		}
		
		d = c;
		c = b;
		b = a;
		a = _mm256_add_epi32(temp1, temp2);
	}
	
	//
	// Output to the hash state values
	//
	_mm256_store_si256(&Context->H[0].u256, _mm256_add_epi32(initialA, a));
	_mm256_store_si256(&Context->H[1].u256, _mm256_add_epi32(initialB, b));
	_mm256_store_si256(&Context->H[2].u256, _mm256_add_epi32(initialC, c));
	_mm256_store_si256(&Context->H[3].u256, _mm256_add_epi32(initialD, d));
	_mm256_store_si256(&Context->H[4].u256, _mm256_add_epi32(initialE, e));
	_mm256_store_si256(&Context->H[5].u256, _mm256_add_epi32(initialF, f));
	_mm256_store_si256(&Context->H[6].u256, _mm256_add_epi32(initialG, g));
	_mm256_store_si256(&Context->H[7].u256, _mm256_add_epi32(initialH, h));
	
	return 0;
}

void
SimdSha256Update(SimdSha2Context* Context, size_t Length, uint8_t* Buffers[])
{
	size_t toWrite = Length;
	size_t bufferIndex;
	size_t bufferOffset;
	size_t next;
	
	next = 0;
	
	while (toWrite > 0)
	{
		bufferIndex = Context->Length / 4;
		
		if ((Context->Length & 0x3) == 0 &&
			toWrite >= 4)
		// 4-byte aligned
		{
			uint32_t** buffer32 = (uint32_t**) Buffers;
			size_t nextIndex = next / 4;
			
			for (size_t lane = 0; lane < Context->Lanes; lane++)
			{
				Context->Buffer[nextIndex].u32[lane] = __builtin_bswap32(buffer32[lane][nextIndex]);
			}
			toWrite -= 4;
			Context->Length += 4;
			Context->BitLength += 32;
			next += 4;
		}
		else
		{
			bufferOffset = Context->Length % 4;
			for (size_t lane = 0; lane < Context->Lanes; lane++)
			{
				uint8_t* bufferPtr = (uint8_t*)&Context->Buffer[bufferIndex].u32[lane];
				bufferPtr[sizeof(uint32_t) - 1 - bufferOffset] = Buffers[lane][next];
			}
			toWrite--;
			Context->Length++;
			Context->BitLength += 8;
			next++;
		}
		
		if (Context->Length == 64)
		{
			SimdSha256Transform(Context);
			Context->Length = 0;
			memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
		}
	}
}

void
SimdSha256Finalize(SimdSha2Context* Context)
{
	//
	// Write the 1 bit
	//
	for (size_t lane = 0; lane < Context->Lanes; lane++)
	{
		size_t bufferIndex = Context->Length / 4;
		size_t bufferOffset = Context->Length % 4;
		uint8_t* bufferPtr = (uint8_t*)&Context->Buffer[bufferIndex].u32[lane];
		bufferPtr[sizeof(uint32_t) - 1 - bufferOffset] = 0x80;
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
	_mm256_store_si256(&Context->Buffer[14].u256, _mm256_set1_epi32(Context->BitLength >> 32));
	_mm256_store_si256(&Context->Buffer[15].u256, _mm256_set1_epi32(Context->BitLength & 0xffffffff));

	SimdSha256Transform(Context);
	
	//
	// Change endianness
	//
	__m256i shufMask = _mm256_setr_epi8(3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12,19,18,17,16,23,22,21,20,27,26,25,24,31,30,29,28);
	__m256i a = _mm256_shuffle_epi8(_mm256_load_si256(&Context->H[0].u256), shufMask);
	__m256i b = _mm256_shuffle_epi8(_mm256_load_si256(&Context->H[1].u256), shufMask);
	__m256i c = _mm256_shuffle_epi8(_mm256_load_si256(&Context->H[2].u256), shufMask);
	__m256i d = _mm256_shuffle_epi8(_mm256_load_si256(&Context->H[3].u256), shufMask);
	__m256i e = _mm256_shuffle_epi8(_mm256_load_si256(&Context->H[4].u256), shufMask);
	__m256i f = _mm256_shuffle_epi8(_mm256_load_si256(&Context->H[5].u256), shufMask);
	__m256i g = _mm256_shuffle_epi8(_mm256_load_si256(&Context->H[6].u256), shufMask);
	__m256i h = _mm256_shuffle_epi8(_mm256_load_si256(&Context->H[7].u256), shufMask);
	
	_mm256_store_si256(&Context->H[0].u256, a);
	_mm256_store_si256(&Context->H[1].u256, b);
	_mm256_store_si256(&Context->H[2].u256, c);
	_mm256_store_si256(&Context->H[3].u256, d);
	_mm256_store_si256(&Context->H[4].u256, e);
	_mm256_store_si256(&Context->H[5].u256, f);
	_mm256_store_si256(&Context->H[6].u256, g);
	_mm256_store_si256(&Context->H[7].u256, h);
}
