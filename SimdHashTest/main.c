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
#include "simdhash.h"

typedef struct _TestVector
{
	size_t 	 Length;
	uint8_t* PreImage;
	uint8_t Digest[SHA256_SIZE];
} TestVector, *PTestVector;

//
// Test vectors
// Source: https://www.di-mgt.com.au/sha_testvectors.html
//
static const TestVector g_TestVectors[] = {
	{0, (uint8_t*)"", {0xe3,0xb0,0xc4,0x42,0x98,0xfc,0x1c,0x14,0x9a,0xfb,0xf4,0xc8,0x99,0x6f,0xb9,0x24,0x27,0xae,0x41,0xe4,0x64,0x9b,0x93,0x4c,0xa4,0x95,0x99,0x1b,0x78,0x52,0xb8,0x55}},
	{3, (uint8_t*)"abc", {0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad}},
	{112, (uint8_t*)"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", {0xcf,0x5b,0x16,0xa7,0x78,0xaf,0x83,0x80,0x03,0x6c,0xe5,0x9e,0x7b,0x04,0x92,0x37,0x0b,0x24,0x9b,0x11,0xe8,0xf0,0x7a,0x51,0xaf,0xac,0x45,0x03,0x7a,0xfe,0xe9,0xd1}}
};

static bool
ToHex(char* Out, size_t OutLength, uint8_t* Buffer, size_t BufferLength)
{
	char* nextOut;
	
	if (OutLength < BufferLength * 2)
	{
		return false;
	}
	
	nextOut = Out;
	for (size_t i = 0; i < BufferLength; i++)
	{
		sprintf(nextOut, "%02x", Buffer[i]);
		nextOut += 2;
	}
	
	return true;
}

static void
FunctionalityTests(void)
/*++
	This function performs the basic functionality
	tests. It uses the known digest values to ensure
	that the algorithms are providing correct results	
--*/
{
	SimdShaContext sha256ctx;
	uint8_t* buffers[SIMD_COUNT];
	uint8_t hash[SHA256_SIZE];
	char hex[SHA256_SIZE * 2 + 1];

	memset(hex, 0, sizeof(hex));

	for (size_t c = 0; c < sizeof(g_TestVectors)/sizeof(*g_TestVectors); c++)
	{
		for (size_t i = 0; i < SIMD_COUNT; i++)
		{
			buffers[i] = (uint8_t*)g_TestVectors[c].PreImage;
		}
		
		SimdSha256Init(&sha256ctx, SIMD_COUNT);
		SimdSha256Update(&sha256ctx, g_TestVectors[c].Length, (const uint8_t**)buffers);
		SimdSha256Finalize(&sha256ctx);
		
		SimdSha256GetHash(&sha256ctx, hash, 0);
		
		if (memcmp(&hash[0], &g_TestVectors[c].Digest[0], SHA256_SIZE) == 0)
		{
			printf("Simd Functional tests passed\n");
			ToHex(hex, sizeof(hex), hash, SHA256_SIZE);
			printf("\t%s\n", hex);
		}
		else
		{
			printf("Simd Functional tests failed\n");
		}

		Sha2Context linearSha2Context;
		Sha256Init(&linearSha2Context);
		Sha256Update(&linearSha2Context, g_TestVectors[c].Length, (const uint8_t*)&g_TestVectors[c].PreImage[0]);
		Sha256Finalize(&linearSha2Context);

		ToHex(hex, sizeof(hex), (uint8_t*)&linearSha2Context.H[0], SHA256_SIZE);
		printf("\t%s\n", hex);

	}

	
}

static struct timespec
timer_start(void)
/*++
	Call this function to start a nanosecond-resolution timer
	Source: https://stackoverflow.com/questions/6749621/how-to-create-a-high-resolution-timer-in-linux-to-measure-program-performance
--*/
{
    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
    return start_time;
}
 
static long
timer_end(
	struct timespec start_time
)
/*++
	Call this function to end a timer, returning nanoseconds elapsed as a long
	Source: https://stackoverflow.com/questions/6749621/how-to-create-a-high-resolution-timer-in-linux-to-measure-program-performance
--*/
{
    struct timespec end_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
    long diffInNanos = (end_time.tv_sec - start_time.tv_sec) * (long)1e9 + (end_time.tv_nsec - start_time.tv_nsec);
    return diffInNanos;
}

static void
PerformanceTests(void)
/*++
	This function measures the performance of the
	digests. It gives a _rough_ measure of hashes
	per second but does not measure correctness.
--*/
{
	SimdShaContext sha256ctx;
	Sha2Context linearSha2Context;
	uint8_t* buffers[SIMD_COUNT];
	struct timespec begin;
	long elapsed;
	size_t hashesPerSec, fastest, slowest, average;
	static const size_t numTests = 1000000;

	//
	// Initialize the buffers
	//
	for (size_t i = 0; i < SIMD_COUNT; i++)
	{
		buffers[i] = (uint8_t*)g_TestVectors[0].PreImage;
	}

	average = 0;
	fastest = 0;
	slowest = SIZE_MAX;

	//
	// Perform tests, capturing statistics
	//
	for (size_t i = 0; i < numTests; i++)
	{
		begin = timer_start();
		SimdSha256Init(&sha256ctx, SIMD_COUNT);
		SimdSha256Update(&sha256ctx, g_TestVectors[0].Length, (const uint8_t**)buffers);
		SimdSha256Finalize(&sha256ctx);
		elapsed = timer_end(begin);

		hashesPerSec = 1e9 / elapsed;

		if (hashesPerSec > fastest)
			fastest = hashesPerSec;
		if (hashesPerSec < slowest)
			slowest = hashesPerSec;
		average += hashesPerSec;
	}

	average /= numTests;

	//
	// Display captured statistics
	//
	printf("SIMD Performance Tests over %zu iterations\n", numTests);
	printf("Fastest (8h/s): %zu\n", fastest);
	printf("Slowest (8h/s): %zu\n", slowest);
	printf("Average (8h/s): %zu\n", average);
	printf("Hashes/core/s : %zu\n", average * 8);

	average = 0;
	fastest = 0;
	slowest = SIZE_MAX;

	//
	// Perform tests, capturing statistics
	//
	for (size_t i = 0; i < numTests; i++)
	{
		begin = timer_start();
		Sha256Init(&linearSha2Context);
		Sha256Update(&linearSha2Context, g_TestVectors[0].Length, (const uint8_t*)&g_TestVectors[0].PreImage[0]);
		Sha256Finalize(&linearSha2Context);
		elapsed = timer_end(begin);

		hashesPerSec = 1e9 / elapsed;

		if (hashesPerSec > fastest)
			fastest = hashesPerSec;
		if (hashesPerSec < slowest)
			slowest = hashesPerSec;
		average += hashesPerSec;
	}

	average /= numTests;

	printf("Linear Performance Tests over %zu iterations\n", numTests);
	printf("Fastest (h/s): %zu\n", fastest);
	printf("Slowest (h/s): %zu\n", slowest);
	printf("Average (h/s): %zu\n", average);
	printf("Hashes/core/s: %zu\n", average);
}

int main(int argc, char* argv[])
{
	FunctionalityTests();
	PerformanceTests();
}
