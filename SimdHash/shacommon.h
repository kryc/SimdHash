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

#include "simdcommon.h"

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
	const uint8_t* Buffers[],
	const uint8_t BigEndian);

static inline simd_t
SimdShaBitwiseChoiceWithControlOriginal(
	const simd_t Choice1,
	const simd_t Choice2,
	const simd_t Control)
/*++
	Version: Original
	Operations: and,  andnot, or
	Latency:     1      1     1     = 3
	Throughput: .33    .5    .33
--*/
{
	// f = (b and c) or ((not b) and d)
	simd_t ctrlAndC1 = and_simd(Control, Choice1);
	simd_t notCtrlAndC2 = andnot_simd(Control, Choice2);
	return or_simd(ctrlAndC1, notCtrlAndC2);
}

static inline simd_t
SimdShaBitwiseChoiceWithControlAlt1(
	const simd_t Choice1,
	const simd_t Choice2,
	const simd_t Control)
/*++
	Version: Alternative 1
	Operations: xor,  and,   xor
	Latency:     1     1      1     = 3
	Throughput: .33   .33    .33
--*/
{
	// f = d xor (b and (c xor d))
	simd_t cXorD = xor_simd(Choice1, Choice2);
	simd_t bAndCXordD = and_simd(Control, cXorD);
	return xor_simd(Choice2, bAndCXordD);
}

static inline simd_t
SimdShaBitwiseChoiceWithControlAlt2(
	const simd_t Choice1,
	const simd_t Choice2,
	const simd_t Control)
/*++
	Version: Alternative 2
	Operations: andnot,  and,   xor
	Latency:      1       1      1     = 3
	Throughput:  .33     .5     .33
--*/
{
	// f = (b and c) xor ((not b) and d)
	simd_t notCtrlAndC2 = andnot_simd(Control, Choice2);
	simd_t ctrlAndC1 = and_simd(Control, Choice1);
	return xor_simd(notCtrlAndC2, ctrlAndC1);
}

static inline simd_t
SimdShaBitwiseMajorityOriginal(
	const simd_t A,
	const simd_t B,
	const simd_t C)
/*++
	Version: Original
	Operations:  and,  and,   and,   xor,   xor
	Latency:      1     1      1      1      1    = 5
	Throughput:  .33   .33    .33    .33    .33
--*/
{
	// f = (b and c) or (b and d) or (c and d) 
	simd_t aAndB = and_simd(A, B);
	simd_t aAndC = and_simd(A, C);
	simd_t bAndC = and_simd(B, C);
	simd_t ret = or_simd(aAndB, aAndC);
	return or_simd(ret, bAndC);
}

static inline simd_t
SimdShaBitwiseMajorityAlt1(
	const simd_t A,
	const simd_t B,
	const simd_t C)
/*++
	Version: Original
	Operations:  and,   or,   and,    or, 
	Latency:      1     1      1      1    = 4
	Throughput:  .33   .33    .33    .33 
--*/
{
	// f = (b and c) or (d and (b or c))
	simd_t bOrC = or_simd(B, C);
	simd_t dAndBOrC = and_simd(A, bOrC);
	simd_t bAndC = and_simd(B, C);
	return or_simd(bAndC, dAndBOrC);
}

static inline simd_t
SimdShaBitwiseMajorityAlt2(
	const simd_t A,
	const simd_t B,
	const simd_t C)
/*++
	Version: Original
	Operations:  and,  xor,   and,    or, 
	Latency:      1     1      1      1    = 4
	Throughput:  .33   .33    .33    .33 
--*/
{
	// f = (b and c) or (d and (b xor c))
	simd_t bXorC = xor_simd(B, C);
	simd_t dAndBXorC = and_simd(A, bXorC);
	simd_t bAndC = and_simd(B, C);
	return or_simd(bAndC, dAndBXorC);
}

static inline simd_t
SimdShaBitwiseMajorityAlt3(
	const simd_t A,
	const simd_t B,
	const simd_t C)
/*++
	Version: Original
	Operations:  and,  xor,   and,   xor, 
	Latency:      1     1      1      1    = 4
	Throughput:  .33   .33    .33    .33 
--*/
{
	// f = (b and c) xor (d and (b xor c))
	simd_t bXorC = xor_simd(B, C);
	simd_t dAndBXorC = and_simd(A, bXorC);
	simd_t bAndC = and_simd(B, C);
	return xor_simd(bAndC, dAndBXorC);
}

static inline simd_t
SimdShaBitwiseMajorityAlt5(
	const simd_t A,
	const simd_t B,
	const simd_t C)
/*++
	Version: Original
	Operations:  and,  and,   and,   xor,  xor
	Latency:      1     1      1      1     1   = 5
	Throughput:  .33   .33    .33    .33   .33
--*/
{
	// (b and c) xor (b and d) xor (c and d)
	simd_t aAndB = and_simd(A, B);
	simd_t aAndC = and_simd(A, C);
	simd_t bAndC = and_simd(B, C);
	simd_t ret = xor_simd(aAndB, aAndC);
	return xor_simd(ret, bAndC);
}

#endif /* shacommon_h */
