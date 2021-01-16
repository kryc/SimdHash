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
#include "simdhash.h"

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

typedef struct _SecondPreimageResult
{
	uint64_t LaneMap : 63;
	uint64_t Result  : 1;
} SecondPreimageResult;

#define SECOND_PREIMAGE_CANDIDATE_FOUND     1
#define SECOND_PREIMAGE_CANDIDATE_NOT_FOUND 0

static inline
__m256i
SimdRotateLeft32(
	const __m256i Value,
	const uint8_t Distance)
{
	assert(Distance < 32);
	__m256i shl = _mm256_slli_epi32(Value, Distance);
	__m256i shr = _mm256_srli_epi32(Value, 32 - Distance);
	return _mm256_or_si256(shl, shr);
}

static inline
__m256i
SimdRotateRight32(
	const __m256i Value,
	const uint8_t Distance)
{
	assert(Distance < 32);
	__m256i shr = _mm256_srli_epi32(Value, Distance);
	__m256i shl = _mm256_slli_epi32(Value, 32 - Distance);
	return _mm256_or_si256(shl, shr);
}

static inline
__m256i
SimdCalculateS1(
	const __m256i E)
{
	__m256i er6 = SimdRotateRight32(E, 6);
	__m256i er11 = SimdRotateRight32(E, 11);
	__m256i er25 = SimdRotateRight32(E, 25);
	__m256i ret = _mm256_xor_si256(er6, er11);
	return _mm256_xor_si256(ret, er25);
}

static inline
__m256i
SimdCalculateS0(
	const __m256i A)
{
	__m256i ar2 = SimdRotateRight32(A, 2);
	__m256i ar13 = SimdRotateRight32(A, 13);
	__m256i ar22 = SimdRotateRight32(A, 22);
	__m256i ret = _mm256_xor_si256(ar2, ar13);
	return _mm256_xor_si256(ret, ar22);
}

static inline
__m256i
SimdCalculateExtendS0(
	const __m256i W)
{
	__m256i wr7 = SimdRotateRight32(W, 7);
	__m256i wr18 = SimdRotateRight32(W, 18);
	__m256i wr3 = _mm256_srli_epi32(W, 3);
	__m256i ret = _mm256_xor_si256(wr7, wr18);
	return _mm256_xor_si256(ret, wr3);
}

static inline
__m256i
SimdCalculateExtendS1(
	const __m256i W)
{
	__m256i wr17 = SimdRotateRight32(W, 17);
	__m256i wr19 = SimdRotateRight32(W, 19);
	__m256i wr10 = _mm256_srli_epi32(W, 10);
	__m256i ret = _mm256_xor_si256(wr17, wr19);
	return _mm256_xor_si256(ret, wr10);
}

static inline
__m256i
SimdCalculateCh(
	const __m256i E,
	const __m256i F,
	const __m256i G)
{
	__m256i eAndF = _mm256_and_si256(E, F);
	__m256i notEAndG = _mm256_andnot_si256(E, G);
	return _mm256_xor_si256(eAndF, notEAndG);
}

static inline
__m256i
SimdCalculateMaj(
	const __m256i A,
	const __m256i B,
	const __m256i C)
{
	__m256i aAndB = _mm256_and_si256(A, B);
	__m256i aAndC = _mm256_and_si256(A, C);
	__m256i bAndC = _mm256_and_si256(B, C);
	__m256i ret = _mm256_xor_si256(aAndB, aAndC);
	return _mm256_xor_si256(ret, bAndC);
}

static inline
__m256i
SimdCalculateTemp2(
	const __m256i A,
	const __m256i B,
	const __m256i C)
{
	__m256i s0 = SimdCalculateS0(A);
	__m256i maj = SimdCalculateMaj(A, B, C);
	return _mm256_add_epi32(s0, maj);
}

static inline
__m256i
SimdCalculateTemp1(
	const __m256i E,
	const __m256i F,
	const __m256i G,
	const __m256i H,
	const __m256i K,
	const __m256i W)
{
	__m256i s1 = SimdCalculateS1(E);
	__m256i ch = SimdCalculateCh(E, F, G);
	__m256i ret = _mm256_add_epi32(H, s1);
	ret = _mm256_add_epi32(ret, ch);
	ret = _mm256_add_epi32(ret, K);
	return _mm256_add_epi32(ret, W);
}

static inline
void
SimdExpandMessageSchedule(
	SimdSha2Context* Context,
	SimdShaValue* messageSchedule)
{
	for (size_t i = 0; i < 16; i++)
	{
		__m256i w = _mm256_load_si256(&Context->Buffer[i].u256);
		_mm256_store_si256(&messageSchedule[i].u256, w);
	}
	
	for (size_t i = 16; i < 64; i++)
	{
		__m256i s0 = SimdCalculateExtendS0(_mm256_load_si256(&messageSchedule[i-15].u256));
		__m256i s1 = SimdCalculateExtendS1(_mm256_load_si256(&messageSchedule[i-2].u256));
		__m256i res = _mm256_add_epi32(_mm256_load_si256(&messageSchedule[i-16].u256), s0);
		res = _mm256_add_epi32(res, _mm256_load_si256(&messageSchedule[i-7].u256));
		res = _mm256_add_epi32(res, s1);
		_mm256_store_si256(&messageSchedule[i].u256, res);
	}
}

extern
void
SimdSha256Init(
	SimdSha2Context* Context,
	const size_t Lanes)
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

static inline void
SimdSha256Transform(
	SimdSha2Context* Context)
{
	//
	// Expand the message schedule
	//
	ALIGN(32) SimdShaValue messageSchedule[64];
	SimdExpandMessageSchedule(Context, messageSchedule);
	
	__m256i a = _mm256_load_si256(&Context->H[0].u256);
	__m256i b = _mm256_load_si256(&Context->H[1].u256);
	__m256i c = _mm256_load_si256(&Context->H[2].u256);
	__m256i d = _mm256_load_si256(&Context->H[3].u256);
	__m256i e = _mm256_load_si256(&Context->H[4].u256);
	__m256i f = _mm256_load_si256(&Context->H[5].u256);
	__m256i g = _mm256_load_si256(&Context->H[6].u256);
	__m256i h = _mm256_load_si256(&Context->H[7].u256);

	//
	// Sha256 compression function
	//
	for (size_t i = 0; i < 64; i++)
	{
		__m256i k = _mm256_set1_epi32(RoundConstants[i]);
		__m256i w = _mm256_load_si256(&messageSchedule[i].u256);
		__m256i temp1 = SimdCalculateTemp1(e, f, g, h, k, w);
		__m256i temp2 = SimdCalculateTemp2(a, b, c);
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
	_mm256_store_si256(&Context->H[0].u256, _mm256_add_epi32(_mm256_load_si256(&Context->H[0].u256), a));
	_mm256_store_si256(&Context->H[1].u256, _mm256_add_epi32(_mm256_load_si256(&Context->H[1].u256), b));
	_mm256_store_si256(&Context->H[2].u256, _mm256_add_epi32(_mm256_load_si256(&Context->H[2].u256), c));
	_mm256_store_si256(&Context->H[3].u256, _mm256_add_epi32(_mm256_load_si256(&Context->H[3].u256), d));
	_mm256_store_si256(&Context->H[4].u256, _mm256_add_epi32(_mm256_load_si256(&Context->H[4].u256), e));
	_mm256_store_si256(&Context->H[5].u256, _mm256_add_epi32(_mm256_load_si256(&Context->H[5].u256), f));
	_mm256_store_si256(&Context->H[6].u256, _mm256_add_epi32(_mm256_load_si256(&Context->H[6].u256), g));
	_mm256_store_si256(&Context->H[7].u256, _mm256_add_epi32(_mm256_load_si256(&Context->H[7].u256), h));
}

static inline
SecondPreimageResult
SimdSha256TransformSecondPreimage(
	SimdSha2SecondPreimageContext* Context)
/*++
	Perform a hash transformation but bail as early as possible if
	the computed DWORDs dont match the target H values.
	Used for finding preimages
 --*/
{
	//
	// Expand the message schedule
	//
	ALIGN(32) SimdShaValue messageSchedule[64];
	SecondPreimageResult result;
	int resultMap;
	
	resultMap = 0;
	
	result.LaneMap = 0;
	result.Result = SECOND_PREIMAGE_CANDIDATE_NOT_FOUND;
	
	SimdExpandMessageSchedule(&Context->ShaContext, messageSchedule);
	
	__m256i a = _mm256_load_si256(&Context->ShaContext.H[0].u256), initialA = a;
	__m256i b = _mm256_load_si256(&Context->ShaContext.H[1].u256), initialB = b;
	__m256i c = _mm256_load_si256(&Context->ShaContext.H[2].u256), initialC = c;
	__m256i d = _mm256_load_si256(&Context->ShaContext.H[3].u256), initialD = d;
	__m256i e = _mm256_load_si256(&Context->ShaContext.H[4].u256), initialE = e;
	__m256i f = _mm256_load_si256(&Context->ShaContext.H[5].u256), initialF = f;
	__m256i g = _mm256_load_si256(&Context->ShaContext.H[6].u256), initialG = g;
	__m256i h = _mm256_load_si256(&Context->ShaContext.H[7].u256), initialH = h;
	
	//
	// Sha256 compression function
	//
	for (size_t i = 0; i < 60; i++)
	{
		__m256i k = _mm256_set1_epi32(RoundConstants[i]);
		__m256i w = _mm256_load_si256(&messageSchedule[i].u256);
		__m256i temp1 = SimdCalculateTemp1(e, f, g, h, k, w);
		__m256i temp2 = SimdCalculateTemp2(a, b, c);
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
	// Preimage attack optimisations for the final
	// four rounds. We keep this out of the main compression
	// function to remove conditions in the fast path
	//
	for (size_t i = 60; i < 64; i++)
	{
		__m256i k = _mm256_set1_epi32(RoundConstants[i]);
		__m256i w = _mm256_load_si256(&messageSchedule[i].u256);
		__m256i temp1 = SimdCalculateTemp1(e, f, g, h, k, w);
		__m256i temp2 = SimdCalculateTemp2(a, b, c);
		h = g;
		g = f;
		f = e;
		e = _mm256_add_epi32(d, temp1);

		if (i == 60)
		{
			__m256i targetH = _mm256_sub_epi32(_mm256_set1_epi32(Context->Target32[7]), initialH);
			__m256i targetD = _mm256_sub_epi32(_mm256_set1_epi32(Context->Target32[3]), initialD);
			__m256i cmpH  = _mm256_cmpeq_epi32(targetH, e);
			__m256i cmpD  = _mm256_cmpeq_epi32(targetD, a);
			__m256i cmpDH = _mm256_and_si256(cmpH, cmpD);
			resultMap = _mm256_movemask_ps((__m256)cmpDH);
			if (resultMap == 0)
				goto exit;
		}
		else if (i == 61)
		{
			__m256i targetG = _mm256_sub_epi32(_mm256_set1_epi32(Context->Target32[6]), initialG);
			__m256i targetC = _mm256_sub_epi32(_mm256_set1_epi32(Context->Target32[2]), initialC);
			__m256i cmpG = _mm256_cmpeq_epi32(targetG, e);
			__m256i cmpC = _mm256_cmpeq_epi32(targetC, a);
			__m256i cmpGC = _mm256_and_si256(cmpG, cmpC);
			resultMap &= _mm256_movemask_ps((__m256)cmpGC);
			if (resultMap == 0)
				goto exit;
		}
		else if (i == 62)
		{
			__m256i targetF = _mm256_sub_epi32(_mm256_set1_epi32(Context->Target32[5]), initialF);
			__m256i targetB = _mm256_sub_epi32(_mm256_set1_epi32(Context->Target32[1]), initialB);
			__m256i cmpF = _mm256_cmpeq_epi32(targetF, e);
			__m256i cmpB = _mm256_cmpeq_epi32(targetB, a);
			__m256i cmpFB = _mm256_and_si256(cmpF, cmpB);
			resultMap &= _mm256_movemask_ps((__m256)cmpFB);
			if (resultMap == 0)
				goto exit;
		}
		else if (i == 63)
		{
			__m256i targetE = _mm256_sub_epi32(_mm256_set1_epi32(Context->Target32[4]), initialE);
			__m256i targetA = _mm256_sub_epi32(_mm256_set1_epi32(Context->Target32[0]), initialA);
			__m256i cmpE = _mm256_cmpeq_epi32(targetE, e);
			__m256i cmpA = _mm256_cmpeq_epi32(targetA, a);
			__m256i cmpEA = _mm256_and_si256(cmpE, cmpA);
			resultMap &= _mm256_movemask_ps((__m256)cmpEA);
			if (resultMap == 0)
				goto exit;
		}
		
		d = c;
		c = b;
		b = a;
		a = _mm256_add_epi32(temp1, temp2);
	}
	
	//
	// If we reach here then we have a match
	// Output to the hash state values
	//
	_mm256_store_si256(&Context->ShaContext.H[0].u256, _mm256_add_epi32(initialA, a));
	_mm256_store_si256(&Context->ShaContext.H[1].u256, _mm256_add_epi32(initialB, b));
	_mm256_store_si256(&Context->ShaContext.H[2].u256, _mm256_add_epi32(initialC, c));
	_mm256_store_si256(&Context->ShaContext.H[3].u256, _mm256_add_epi32(initialD, d));
	_mm256_store_si256(&Context->ShaContext.H[4].u256, _mm256_add_epi32(initialE, e));
	_mm256_store_si256(&Context->ShaContext.H[5].u256, _mm256_add_epi32(initialF, f));
	_mm256_store_si256(&Context->ShaContext.H[6].u256, _mm256_add_epi32(initialG, g));
	_mm256_store_si256(&Context->ShaContext.H[7].u256, _mm256_add_epi32(initialH, h));

	result.Result  = SECOND_PREIMAGE_CANDIDATE_FOUND;
	result.LaneMap = resultMap;
	
exit:
	
	return result;
}

static inline
void
SimdWriteSingleByte(
	SimdSha2Context* Context,
	const size_t Lane,
	const size_t Buffer,
	const size_t Offset,
	const uint8_t Byte)
{
	uint8_t* bufferPtr = (uint8_t*)&Context->Buffer[Buffer].u32[Lane];
	bufferPtr[sizeof(uint32_t) - 1 - Offset] = Byte;
}

void
SimdSha256Update(
	SimdSha2Context* Context,
	const size_t Length,
	const uint8_t* Buffers[])
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
			size_t nextInputIndex = next / 4;
			
			for (size_t lane = 0; lane < Context->Lanes; lane++)
			{
				Context->Buffer[bufferIndex].u32[lane] = __builtin_bswap32(buffer32[lane][nextInputIndex]);
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
				SimdWriteSingleByte(Context, lane, bufferIndex, bufferOffset, Buffers[lane][next]);
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

static inline
void
SimdSha256AppendSize(
	SimdSha2Context* Context)
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
		SimdWriteSingleByte(Context, lane, bufferIndex, bufferOffset, 0x80);
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
}

void
SimdSha256Finalize(
	SimdSha2Context* Context)
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

extern
void
SimdSha256SecondPreimageInit(
	SimdSha2SecondPreimageContext* Context,
	SimdSha2Context* ShaContext,
	const uint8_t* Target)
{
	Context->ShaContext = *ShaContext;
	memcpy(&Context->Target8, Target, SHA256_SIZE);
	memset(&Context->Target32[0], 0, sizeof(Context->Target32));
	
	uint32_t* t32 = (uint32_t*)&Context->Target8[0];
	Context->Target32[0] = __builtin_bswap32(t32[0]);
	Context->Target32[1] = __builtin_bswap32(t32[1]);
	Context->Target32[2] = __builtin_bswap32(t32[2]);
	Context->Target32[3] = __builtin_bswap32(t32[3]);
	Context->Target32[4] = __builtin_bswap32(t32[4]);
	Context->Target32[5] = __builtin_bswap32(t32[5]);
	Context->Target32[6] = __builtin_bswap32(t32[6]);
	Context->Target32[7] = __builtin_bswap32(t32[7]);
}

size_t
SimdSha256SecondPreimage(
	SimdSha2SecondPreimageContext* Context,
	const size_t Length,
	const uint8_t* Buffers[])
{
	SecondPreimageResult result;
	
	SimdSha256Update(&Context->ShaContext, Length, Buffers);
	SimdSha256AppendSize(&Context->ShaContext);
	result = SimdSha256TransformSecondPreimage(Context);
	
	if (result.Result == SECOND_PREIMAGE_CANDIDATE_FOUND)
	{
		//
		// Change endianness
		//
		__m256i shufMask = _mm256_setr_epi8(3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12,19,18,17,16,23,22,21,20,27,26,25,24,31,30,29,28);
		__m256i a = _mm256_shuffle_epi8(_mm256_load_si256(&Context->ShaContext.H[0].u256), shufMask);
		__m256i b = _mm256_shuffle_epi8(_mm256_load_si256(&Context->ShaContext.H[1].u256), shufMask);
		__m256i c = _mm256_shuffle_epi8(_mm256_load_si256(&Context->ShaContext.H[2].u256), shufMask);
		__m256i d = _mm256_shuffle_epi8(_mm256_load_si256(&Context->ShaContext.H[3].u256), shufMask);
		__m256i e = _mm256_shuffle_epi8(_mm256_load_si256(&Context->ShaContext.H[4].u256), shufMask);
		__m256i f = _mm256_shuffle_epi8(_mm256_load_si256(&Context->ShaContext.H[5].u256), shufMask);
		__m256i g = _mm256_shuffle_epi8(_mm256_load_si256(&Context->ShaContext.H[6].u256), shufMask);
		__m256i h = _mm256_shuffle_epi8(_mm256_load_si256(&Context->ShaContext.H[7].u256), shufMask);
		
		_mm256_store_si256(&Context->ShaContext.H[0].u256, a);
		_mm256_store_si256(&Context->ShaContext.H[1].u256, b);
		_mm256_store_si256(&Context->ShaContext.H[2].u256, c);
		_mm256_store_si256(&Context->ShaContext.H[3].u256, d);
		_mm256_store_si256(&Context->ShaContext.H[4].u256, e);
		_mm256_store_si256(&Context->ShaContext.H[5].u256, f);
		_mm256_store_si256(&Context->ShaContext.H[6].u256, g);
		_mm256_store_si256(&Context->ShaContext.H[7].u256, h);
		
		//
		// Translate lane mask to lane index
		//
		for(size_t i = 0; i < SIMD_COUNT; i++)
		{
			size_t mask = 1 << i;
			if ((result.LaneMap & mask) == mask)
			{
				return i;
			}
		}
	}
	
	return (size_t)-1;
}

void SimdSha256GetHash(
	SimdSha2Context* Context,
	uint8_t* HashBuffer,
	const size_t Lane)
{
	uint32_t* nextBuffer;
	
	nextBuffer = (uint32_t*)HashBuffer;
	nextBuffer[0] = Context->H[0].u32[Lane];
	nextBuffer[1] = Context->H[1].u32[Lane];
	nextBuffer[2] = Context->H[2].u32[Lane];
	nextBuffer[3] = Context->H[3].u32[Lane];
	nextBuffer[4] = Context->H[4].u32[Lane];
	nextBuffer[5] = Context->H[5].u32[Lane];
	nextBuffer[6] = Context->H[6].u32[Lane];
	nextBuffer[7] = Context->H[7].u32[Lane];
}

void SimdSha256GetHashes(
	SimdSha2Context* Context,
	uint8_t** HashBuffers)
{
	for (size_t i = 0; i < Context->Lanes; i++)
	{
		SimdSha256GetHash(Context, HashBuffers[i], i);
	}
}

void Sha256Init(
	Sha2Context* Context)
{
	for (size_t i = 0; i < 8; i++)
		Context->H[i] = InitialValues[i];
	Context->Length = 0;
	Context->BitLength = 0;
	memset(&Context->Buffer[0], 0, sizeof(Context->Buffer));
}

static inline
uint32_t
RotateRight32(
	const uint32_t Value,
	const size_t Distance)
{
	return (Value >> Distance) | (Value << (32 - Distance));
}

static inline
uint32_t
CalculateExtendS0(
	const uint32_t W)
{
	uint32_t wr7 = RotateRight32(W, 7);
	uint32_t wr18 = RotateRight32(W, 18);
	uint32_t wr3 = W >> 3;
	return wr7 ^ wr18 ^ wr3;
}

static inline
uint32_t
CalculateExtendS1(
	const uint32_t W)
{
	uint32_t wr17 = RotateRight32(W, 17);
	uint32_t wr19 = RotateRight32(W, 19);
	uint32_t wr10 = W >> 10;
	return wr17 ^ wr19 ^ wr10;
}

static inline
void
ExpandMessageSchedule(
	Sha2Context* Context,
	uint32_t* messageSchedule)
{
	for (size_t i = 0; i < 16; i++)
	{
		messageSchedule[i] = Context->Buffer[i];
	}
	
	for (size_t i = 16; i < 64; i++)
	{
		uint32_t s0 = CalculateExtendS0(messageSchedule[i-15]);
		uint32_t s1 = CalculateExtendS1(messageSchedule[i-2]);
		messageSchedule[i] = messageSchedule[i-16] + s0 + messageSchedule[i-7] + s1;
	}
}

static inline
uint32_t
CalculateS1(
	const uint32_t E)
{
	uint32_t er6 = RotateRight32(E, 6);
	uint32_t er11 = RotateRight32(E, 11);
	uint32_t er25 = RotateRight32(E, 25);
	return er6 ^ er11 ^ er25;
}

static inline
uint32_t
CalculateS0(
	const uint32_t A)
{
	uint32_t ar2 = RotateRight32(A, 2);
	uint32_t ar13 = RotateRight32(A, 13);
	uint32_t ar22 = RotateRight32(A, 22);
	return ar2 ^ ar13 ^ ar22;
}

static inline
uint32_t
CalculateCh(
	const uint32_t E,
	const uint32_t F,
	const uint32_t G)
{
	uint32_t eAndF = (E & F);
	uint32_t notEAndG = ((~E) & G);
	return eAndF ^ notEAndG;
}

static inline
uint32_t
CalculateMaj(
	const uint32_t A,
	const uint32_t B,
	const uint32_t C)
{
	uint32_t aAndB = (A & B);
	uint32_t aAndC = (A & C);
	uint32_t bAndC = (B & C);
	return aAndB ^ aAndC ^ bAndC;
}

static inline
uint32_t
CalculateTemp2(
	const uint32_t A,
	const uint32_t B,
	const uint32_t C)
{
	uint32_t s0 = CalculateS0(A);
	uint32_t maj = CalculateMaj(A, B, C);
	return s0 + maj;
}

static inline
uint32_t
CalculateTemp1(
	const uint32_t E,
	const uint32_t F,
	const uint32_t G,
	const uint32_t H,
	const uint32_t K,
	const uint32_t W)
{
	uint32_t s1 = CalculateS1(E);
	uint32_t ch = CalculateCh(E, F, G);
	return H + s1 + ch + K + W;
}

static inline void
Sha256Transform(
	Sha2Context* Context)
{
	//
	// Expand the message schedule
	//
	uint32_t messageSchedule[64];
	ExpandMessageSchedule(Context, messageSchedule);
	
	uint32_t a = Context->H[0];
	uint32_t b = Context->H[1];
	uint32_t c = Context->H[2];
	uint32_t d = Context->H[3];
	uint32_t e = Context->H[4];
	uint32_t f = Context->H[5];
	uint32_t g = Context->H[6];
	uint32_t h = Context->H[7];

	//
	// Sha256 compression function
	//
	for (size_t i = 0; i < 64; i++)
	{
		uint32_t k = RoundConstants[i];
		uint32_t w = messageSchedule[i];
		uint32_t temp1 = CalculateTemp1(e, f, g, h, k, w);
		uint32_t temp2 = CalculateTemp2(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + temp1;
		d = c;
		c = b;
		b = a;
		a = temp1 + temp2;
	}
	
	//
	// Output to the hash state values
	//
	Context->H[0] += a;
	Context->H[1] += b;
	Context->H[2] += c;
	Context->H[3] += d;
	Context->H[4] += e;
	Context->H[5] += f;
	Context->H[6] += g;
	Context->H[7] += h;
}

static inline
void
WriteSingleByte(
	Sha2Context* Context,
	const size_t Buffer,
	const size_t Offset,
	const uint8_t Byte)
{
	uint8_t* bufferPtr = (uint8_t*)&Context->Buffer[Buffer];
	bufferPtr[sizeof(uint32_t) - 1 - Offset] = Byte;
}

void Sha256Update(
	Sha2Context* Context,
	const size_t Length,
	const uint8_t* Buffer)
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
			uint32_t* buffer32 = (uint32_t*) Buffer;
			size_t nextInputIndex = next / 4;
			Context->Buffer[bufferIndex] = __builtin_bswap32(buffer32[nextInputIndex]);
			toWrite -= 4;
			Context->Length += 4;
			Context->BitLength += 32;
			next += 4;
		}
		else
		{
			bufferOffset = Context->Length % 4;
			WriteSingleByte(Context, bufferIndex, bufferOffset, Buffer[next]);
			toWrite--;
			Context->Length++;
			Context->BitLength += 8;
			next++;
		}

		if (Context->Length == 64)
		{
			Sha256Transform(Context);
			Context->Length = 0;
			memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
		}
	}
}

static inline
void
Sha256AppendSize(
	Sha2Context* Context)
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
	WriteSingleByte(Context, bufferIndex, bufferOffset, 0x80);
	Context->Length++;
	
	if (Context->Length >= 56)
	{
		Sha256Transform(Context);
		memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
	}
	
	//
	// Write the length into the last 64 bits
	//
	Context->Buffer[14] = Context->BitLength >> 32;
	Context->Buffer[15] = Context->BitLength & 0xffffffff;
}

void Sha256Finalize(
	Sha2Context* Context)
{
	//
	// Add the message length
	//
	Sha256AppendSize(Context);

	//
	// Compute the final transformation
	//
	Sha256Transform(Context);
	
	//
	// Change endianness
	//
	Context->H[0] = __builtin_bswap32(Context->H[0]);
	Context->H[1] = __builtin_bswap32(Context->H[1]);
	Context->H[2] = __builtin_bswap32(Context->H[2]);
	Context->H[3] = __builtin_bswap32(Context->H[3]);
	Context->H[4] = __builtin_bswap32(Context->H[4]);
	Context->H[5] = __builtin_bswap32(Context->H[5]);
	Context->H[6] = __builtin_bswap32(Context->H[6]);
	Context->H[7] = __builtin_bswap32(Context->H[7]);
}
