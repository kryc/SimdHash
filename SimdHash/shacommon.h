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

size_t
SimdShaUpdateBuffer(
	SimdShaContext* Context,
	const size_t Offset,
	const size_t Length,
	const uint8_t* Buffers[]);

inline __m256i
SimdShaBitwiseChoiceWithControl(
	const __m256i Choice1,
	const __m256i Choice2,
	const __m256i Control)
{
	__m256i ctrlAndC1 = _mm256_and_si256(Control, Choice1);
	__m256i notCtrlAndC2 = _mm256_andnot_si256(Control, Choice2);
	return _mm256_or_si256(ctrlAndC1, notCtrlAndC2);
}

inline __m256i
SimdShaBitwiseMajority(
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

#endif /* shacommon_h */
