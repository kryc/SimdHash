//
//  shacommon.h
//  SimdHash
//
//  Created by Gareth Evans on 20/01/2021.
//  Copyright © 2021 Gareth Evans. All rights reserved.
//

#ifndef shacommon_h
#define shacommon_h

#include <immintrin.h>

#ifndef SHABITWISECHOICE
#define SHABITWISECHOICE 0
#endif

#ifndef SHABITWISEMAJORITY
#define SHABITWISEMAJORITY 2
#endif

#if SHABITWISECHOICE == 1
	#define SimdShaBitwiseChoiceWithControl SimdShaBitwiseChoiceWithControlAlt1
#elif SHABITWISECHOICE == 2
	#define SimdShaBitwiseChoiceWithControl SimdShaBitwiseChoiceWithControlAlt2
#else
	#define SimdShaBitwiseChoiceWithControl SimdShaBitwiseChoiceWithControlOriginal
#endif

#if SHABITWISEMAJORITY == 1
	#define SimdShaBitwiseMajority SimdShaBitwiseMajorityAlt1
#elif SHABITWISEMAJORITY == 2
	#define SimdShaBitwiseMajority SimdShaBitwiseMajorityAlt2
#elif SHABITWISEMAJORITY == 3
	#define SimdShaBitwiseMajority SimdShaBitwiseMajorityAlt3
#elif SHABITWISEMAJORITY == 5
	#define SimdShaBitwiseMajority SimdShaBitwiseMajorityAlt5
#else
	#define SimdShaBitwiseMajority SimdShaBitwiseMajorityOriginal
#endif

size_t
SimdShaUpdateBuffer(
	SimdShaContext* Context,
	const size_t Offset,
	const size_t Length,
	const uint8_t* Buffers[]);

static inline __m256i
SimdShaBitwiseChoiceWithControlOriginal(
	const __m256i Choice1,
	const __m256i Choice2,
	const __m256i Control)
/*++
	Version: Original
	Operations: and,  andnot, or
	Latency:     1      1     1     = 3
	Throughput: .33    .5    .33
--*/
{
	// f = (b and c) or ((not b) and d)
	__m256i ctrlAndC1 = _mm256_and_si256(Control, Choice1);
	__m256i notCtrlAndC2 = _mm256_andnot_si256(Control, Choice2);
	return _mm256_or_si256(ctrlAndC1, notCtrlAndC2);
}

static inline __m256i
SimdShaBitwiseChoiceWithControlAlt1(
	const __m256i Choice1,
	const __m256i Choice2,
	const __m256i Control)
/*++
	Version: Alternative 1
	Operations: xor,  and,   xor
	Latency:     1     1      1     = 3
	Throughput: .33   .33    .33
--*/
{
	// f = d xor (b and (c xor d))
	__m256i cXorD = _mm256_xor_si256(Choice1, Choice2);
	__m256i bAndCXordD = _mm256_and_si256(Control, cXorD);
	return _mm256_xor_si256(Choice2, bAndCXordD);
}

static inline __m256i
SimdShaBitwiseChoiceWithControlAlt2(
	const __m256i Choice1,
	const __m256i Choice2,
	const __m256i Control)
/*++
	Version: Alternative 2
	Operations: andnot,  and,   xor
	Latency:      1       1      1     = 3
	Throughput:  .33     .5     .33
--*/
{
	// f = (b and c) xor ((not b) and d)
	__m256i notCtrlAndC2 = _mm256_andnot_si256(Control, Choice2);
	__m256i ctrlAndC1 = _mm256_and_si256(Control, Choice1);
	return _mm256_xor_si256(notCtrlAndC2, ctrlAndC1);
}

static inline __m256i
SimdShaBitwiseMajorityOriginal(
	const __m256i A,
	const __m256i B,
	const __m256i C)
/*++
	Version: Original
	Operations:  and,  and,   and,   xor,   xor
	Latency:      1     1      1      1      1    = 5
	Throughput:  .33   .33    .33    .33    .33
--*/
{
	// f = (b and c) or (b and d) or (c and d) 
	__m256i aAndB = _mm256_and_si256(A, B);
	__m256i aAndC = _mm256_and_si256(A, C);
	__m256i bAndC = _mm256_and_si256(B, C);
	__m256i ret = _mm256_or_si256(aAndB, aAndC);
	return _mm256_or_si256(ret, bAndC);
}

static inline __m256i
SimdShaBitwiseMajorityAlt1(
	const __m256i A,
	const __m256i B,
	const __m256i C)
/*++
	Version: Original
	Operations:  and,   or,   and,    or, 
	Latency:      1     1      1      1    = 4
	Throughput:  .33   .33    .33    .33 
--*/
{
	// f = (b and c) or (d and (b or c))
	__m256i bOrC = _mm256_or_si256(B, C);
	__m256i dAndBOrC = _mm256_and_si256(A, bOrC);
	__m256i bAndC = _mm256_and_si256(B, C);
	return _mm256_or_si256(bAndC, dAndBOrC);
}

static inline __m256i
SimdShaBitwiseMajorityAlt2(
	const __m256i A,
	const __m256i B,
	const __m256i C)
/*++
	Version: Original
	Operations:  and,  xor,   and,    or, 
	Latency:      1     1      1      1    = 4
	Throughput:  .33   .33    .33    .33 
--*/
{
	// f = (b and c) or (d and (b xor c))
	__m256i bXorC = _mm256_xor_si256(B, C);
	__m256i dAndBXorC = _mm256_and_si256(A, bXorC);
	__m256i bAndC = _mm256_and_si256(B, C);
	return _mm256_or_si256(bAndC, dAndBXorC);
}

static inline __m256i
SimdShaBitwiseMajorityAlt3(
	const __m256i A,
	const __m256i B,
	const __m256i C)
/*++
	Version: Original
	Operations:  and,  xor,   and,   xor, 
	Latency:      1     1      1      1    = 4
	Throughput:  .33   .33    .33    .33 
--*/
{
	// f = (b and c) xor (d and (b xor c))
	__m256i bXorC = _mm256_xor_si256(B, C);
	__m256i dAndBXorC = _mm256_and_si256(A, bXorC);
	__m256i bAndC = _mm256_and_si256(B, C);
	return _mm256_xor_si256(bAndC, dAndBXorC);
}

static inline __m256i
SimdShaBitwiseMajorityAlt5(
	const __m256i A,
	const __m256i B,
	const __m256i C)
/*++
	Version: Original
	Operations:  and,  and,   and,   xor,  xor
	Latency:      1     1      1      1     1   = 5
	Throughput:  .33   .33    .33    .33   .33
--*/
{
	// (b and c) xor (b and d) xor (c and d)
	__m256i aAndB = _mm256_and_si256(A, B);
	__m256i aAndC = _mm256_and_si256(A, C);
	__m256i bAndC = _mm256_and_si256(B, C);
	__m256i ret = _mm256_xor_si256(aAndB, aAndC);
	return _mm256_xor_si256(ret, bAndC);
}

#endif /* shacommon_h */
