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

static const uint8_t g_TestData[] = "AAAAAAAAAAAAAAAA";
static const uint8_t g_TestExpected[] = {0x99,0x12,0x04,0xfb,0xa2,0xb6,0x21,0x6d,0x47,0x62,0x82,0xd3,0x75,0xab,0x88,0xd2,0x0e,0x61,0x08,0xd1,0x09,0xae,0xcd,0xed,0x97,0xef,0x42,0x4d,0xdd,0x11,0x47,0x06};

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
	SimdSha2Context sha256ctx;
	uint8_t* buffers[SIMD_COUNT];
	uint8_t hash[SHA256_SIZE];
	char hex[SHA256_SIZE * 2 + 1];

	memset(hex, 0, sizeof(hex));

	for (size_t i = 0; i < SIMD_COUNT; i++)
	{
		buffers[i] = (uint8_t*)g_TestData;
	}
	
	SimdSha256Init(&sha256ctx, SIMD_COUNT);
	SimdSha256Update(&sha256ctx, strlen((char*)g_TestData), (const uint8_t**)buffers);
	SimdSha256Finalize(&sha256ctx);
	
	SimdSha256GetHash(&sha256ctx, hash, 0);
	
	if (memcmp(&hash[0], &g_TestExpected[0], sizeof(g_TestExpected)) == 0)
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
	Sha256Update(&linearSha2Context, strlen((char*)g_TestData), (const uint8_t*)&g_TestData[0]);
	Sha256Finalize(&linearSha2Context);

	printf("\t%08x%08x%08x%08x%08x%08x%08x%08x\n",
		linearSha2Context.H[0],
		linearSha2Context.H[1],
		linearSha2Context.H[2],
		linearSha2Context.H[3],
		linearSha2Context.H[4],
		linearSha2Context.H[5],
		linearSha2Context.H[6],
		linearSha2Context.H[7]);
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
	SimdSha2Context sha256ctx;
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
		buffers[i] = (uint8_t*)g_TestData;
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
		SimdSha256Update(&sha256ctx, strlen((char*)g_TestData), (const uint8_t**)buffers);
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
		Sha256Update(&linearSha2Context, strlen((char*)g_TestData), (const uint8_t*)&g_TestData[0]);
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
