//
//  main.c
//  SimdHashTest
//
//  Created by Gareth Evans on 07/01/2021.
//  Copyright © 2021 Gareth Evans. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>
#include <immintrin.h>	// AVX
#include <inttypes.h>

#include "simdhash.h"
#include "simdcommon.h"
#include "hashcommon.h"

static inline
uint32_t
RotateRight32(
	const uint32_t Value,
	const size_t Distance)
{
	return (Value >> Distance) | (Value << (32 - Distance));
}

static uint32_t
CalculateExtendS0(
	const uint32_t W)
{
	uint32_t wr7 = RotateRight32(W, 7);
	uint32_t wr18 = RotateRight32(W, 18);
	uint32_t wr3 = W >> 3;
	return wr7 ^ wr18 ^ wr3;
}

static uint32_t
CalculateExtendS1(
	const uint32_t W)
{
	uint32_t wr17 = RotateRight32(W, 17);
	uint32_t wr19 = RotateRight32(W, 19);
	uint32_t wr10 = W >> 10;
	return wr17 ^ wr19 ^ wr10;
}

static uint32_t
CalculateS0(
	const uint32_t A)
{
	uint32_t ar2 = RotateRight32(A, 2);
	uint32_t ar13 = RotateRight32(A, 13);
	uint32_t ar22 = RotateRight32(A, 22);
	return ar2 ^ ar13 ^ ar22;
}

static uint32_t
CalculateS1(
	const uint32_t E)
{
	uint32_t er6 = RotateRight32(E, 6);
	uint32_t er11 = RotateRight32(E, 11);
	uint32_t er25 = RotateRight32(E, 25);
	return er6 ^ er11 ^ er25;
}

static uint32_t
CalculateCh(
	const uint32_t E,
	const uint32_t F,
	const uint32_t G)
{
	uint32_t eAndF = (E & F);
	uint32_t notEAndG = ((~E) & G);
	return eAndF ^ notEAndG;
}

static uint32_t
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

static uint32_t
CalculateTemp2(
	const uint32_t A,
	const uint32_t B,
	const uint32_t C)
{
	uint32_t s0 = CalculateS0(A);
	uint32_t maj = CalculateMaj(A, B, C);
	return s0 + maj;
}

static uint32_t
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

int
TestEqual32(
    const SimdValue Simd,
    const uint32_t Value 
)
{
    for (
        size_t i = 0;
        i < sizeof(Simd.epi32_u32) / sizeof(uint32_t);
        i++
    )
    {
        if (Simd.epi32_u32[i] != Value)
        {
            return 1;
        }
    }
    return 0;
}

int
RunHashTests(
    const int Iteration,
    const uint32_t TestVal
)
{
    SimdValue res;
    simd_t test, s0t;
    int ret;
    uint32_t u32;

    ret = 0;

    test = set1_epi32(TestVal);

    printf("Running test #%d\n", Iteration);

    printf("CalculateS0: ");
    u32 = CalculateS0(TestVal);
    s0t = SimdCalculateS0(test);
    store_simd(&res.usimd, s0t);
    ret ^= u32 ^ res.epi32_u32[0];
    TestEqual32(res, u32) == 0 ? printf("Pass\n") : printf("Fail\n");

    printf("CalculateS1: ");
    u32 = CalculateS1(TestVal);
    s0t = SimdCalculateS1(test);
    store_simd(&res.usimd, s0t);
    ret ^= u32 ^ res.epi32_u32[0];
    TestEqual32(res, u32) == 0 ? printf("Pass\n") : printf("Fail\n");

    printf("CalculateExtendS0: ");
    u32 = CalculateExtendS0(TestVal);
    s0t = SimdCalculateExtendS0(test);
    store_simd(&res.usimd, s0t);
    ret ^= u32 ^ res.epi32_u32[0];
    TestEqual32(res, u32) == 0 ? printf("Pass\n") : printf("Fail\n");

    printf("CalculateExtendS1: ");
    u32 = CalculateExtendS1(TestVal);
    s0t = SimdCalculateExtendS1(test);
    store_simd(&res.usimd, s0t);
    ret ^= u32 ^ res.epi32_u32[0];
    TestEqual32(res, u32) == 0 ? printf("Pass\n") : printf("Fail\n");

    printf("CalculateCh: ");
    u32 = CalculateCh(TestVal, TestVal, TestVal);
    s0t = SimdBitwiseChoiceWithControl(test, test, test);
    store_simd(&res.usimd, s0t);
    ret ^= u32 ^ res.epi32_u32[0];
    TestEqual32(res, u32) == 0 ? printf("Pass\n") : printf("Fail\n");

    printf("CalculateMaj: ");
    u32 = CalculateMaj(TestVal, TestVal, TestVal);
    s0t = SimdBitwiseMajority(test, test, test);
    store_simd(&res.usimd, s0t);
    ret ^= u32 ^ res.epi32_u32[0];
    TestEqual32(res, u32) == 0 ? printf("Pass\n") : printf("Fail\n");

    printf("CalculateTemp1: ");
    u32 = CalculateTemp1(TestVal, TestVal, TestVal, TestVal, TestVal, TestVal);
    s0t = SimdCalculateTemp1(test, test, test, test, test, test);
    store_simd(&res.usimd, s0t);
    ret ^= u32 ^ res.epi32_u32[0];
    TestEqual32(res, u32) == 0 ? printf("Pass\n") : printf("Fail\n");

    printf("CalculateTemp2: ");
    u32 = CalculateTemp2(TestVal, TestVal, TestVal);
    s0t = SimdCalculateTemp2(test, test, test);
    store_simd(&res.usimd, s0t);
    ret ^= u32 ^ res.epi32_u32[0];
    TestEqual32(res, u32) == 0 ? printf("Pass\n") : printf("Fail\n");

    return ret;
}

int
RunLibraryTests(
    void
)
{
    printf("Running library code tests\n");

    printf("\tSimdLanes: ");

#if defined __AVX512BW__
    SimdLanes() == 16 ? printf("Pass (%zu)\n", SimdLanes()) : printf("Fail (%zu)\n", SimdLanes());
#elif defined __AVX2__
    SimdLanes() == 8 ? printf("Pass (%zu)\n", SimdLanes()) : printf("Fail (%zu)\n", SimdLanes());
#endif

    SimdValue res;

    printf("\txmul_epu32: ");
    simd_t z = xmul_epu32(set1_epi32(128), set1_epi32(10));
    store_simd(&res.usimd, z);
    TestEqual32(res, 128 * 10) == 0 ? printf("Pass\n") : printf("Fail\n");

    printf("\tmod2_epi32: ");
    z = mod2_epi32(set1_epi32(1600), 16);
    store_simd(&res.usimd, z);
    TestEqual32(res, 1600 % 16) == 0 ? printf("Pass\n") : printf("Fail\n");

    printf("\tmod2_epi32: ");
    z = mod2_epi32(set1_epi32(1601), 16);
    store_simd(&res.usimd, z);
    TestEqual32(res, 1601 % 16) == 0 ? printf("Pass\n") : printf("Fail\n");

    printf("\tnot_simd: ");
    z = not_simd(set1_epi32(0x41414141));
    store_simd(&res.usimd, z);
    TestEqual32(res, ~0x41414141) == 0 ? printf("Pass\n") : printf("Fail\n");

    return 0;
}

int main(
    int argc,
    char* argv[]
)
{
    uint32_t random_test;
    int ret;

    ret = 0;

    RunLibraryTests();

    srand(time(NULL));

    for(int i = 0; i < 1; i++)
    {
        random_test = (uint32_t)rand();
        ret ^= RunHashTests(i, random_test);
    }
    
    if (ret == 0)
    {
        printf("All tests passed\n");
    }
    else
    {
        printf("Test failure detected\n");
    }

    return 0;
}