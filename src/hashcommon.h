//
//  hashcommon.h
//  SimdHash
//
//  Created by Gareth Evans on 20/01/2021.
//  Copyright © 2021 Gareth Evans. All rights reserved.
//

#ifndef hashcommon_h
#define hashcommon_h

#include "simdcommon.h"

#ifndef BITWISECHOICE
#define BITWISECHOICE 0
#endif

#ifndef BITWISEMAJORITY
#define BITWISEMAJORITY 2
#endif

#if BITWISECHOICE == 1
	#define SimdBitwiseChoiceWithControl SimdBitwiseChoiceWithControlAlt1
#elif BITWISECHOICE == 2
	#define SimdBitwiseChoiceWithControl SimdBitwiseChoiceWithControlAlt2
#else
	#define SimdBitwiseChoiceWithControl SimdBitwiseChoiceWithControlOriginal
#endif

#if BITWISEMAJORITY == 1
	#define SimdBitwiseMajority SimdBitwiseMajorityAlt1
#elif BITWISEMAJORITY == 2
	#define SimdBitwiseMajority SimdBitwiseMajorityAlt2
#elif BITWISEMAJORITY == 3
	#define SimdBitwiseMajority SimdBitwiseMajorityAlt3
#elif BITWISEMAJORITY == 5
	#define SimdBitwiseMajority SimdBitwiseMajorityAlt5
#else
	#define SimdBitwiseMajority SimdBitwiseMajorityOriginal
#endif

size_t
SimdHashUpdateLaneBuffer(
	SimdHashContext* Context,
	const size_t Lane,
	const size_t Length,
	const uint8_t* Buffers);

// size_t
// SimdHashUpdateBuffer(
// 	SimdHashContext* Context,
// 	const size_t Offset,
// 	const size_t Length,
// 	const uint8_t* Buffers[],
// 	const uint8_t BigEndian);

static inline simd_t
SimdBitwiseChoiceWithControlOriginal(
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
SimdBitwiseChoiceWithControlAlt1(
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
SimdBitwiseChoiceWithControlAlt2(
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
SimdBitwiseMajorityOriginal(
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
SimdBitwiseMajorityAlt1(
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
SimdBitwiseMajorityAlt2(
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
SimdBitwiseMajorityAlt3(
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
SimdBitwiseMajorityAlt5(
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

#endif /* hashcommon_h */
