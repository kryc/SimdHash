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
PerformanceTests(
    const HashAlgorithm Algorithm,
    const size_t Iterations
)
/*++
	This function measures the performance of the
	digests. It gives a _rough_ measure of hashes
	per second but does not measure correctness.
--*/
{
	SimdHashContext context;
	uint8_t* buffers[SimdLanes()];
	struct timespec begin;
	long elapsed;
	size_t hashesPerSec, fastest, slowest, average;
    static const uint8_t input[128] = {0};
    const char* algorithmString;

	//
	// Initialize the buffers
	//
	for (size_t i = 0; i < SimdLanes(); i++)
	{
		buffers[i] = (uint8_t*)input;
	}

	average = 0;
	fastest = 0;
	slowest = SIZE_MAX;

    algorithmString = HashAlgorithmToString(Algorithm);

    for (size_t i = 0; i < Iterations; i++)
    {
        begin = timer_start();
        SimdHashInit(&context, Algorithm);
        SimdHashUpdateAll(&context, 48 /*Semi-random length*/, (const uint8_t**)buffers);
        SimdHashFinalize(&context);
        elapsed = timer_end(begin);

        hashesPerSec = 1e9 / elapsed;

        if (hashesPerSec > fastest)
            fastest = hashesPerSec;
        if (hashesPerSec < slowest)
            slowest = hashesPerSec;
        average += hashesPerSec;
    }

    average /= Iterations;

    //
    // Display captured statistics
    //
    printf("SIMD %s Performance Tests over %zu iterations\n", algorithmString, Iterations);
    printf("Number of SIMD lanes: %zu\n", SimdLanes());
    printf("  Fastest (%zuh/s): %zu\n", SimdLanes(), fastest);
    printf("  Slowest (%zuh/s): %zu\n", SimdLanes(), slowest);
    printf("  Average (%zuh/s): %zu\n", SimdLanes(), average);
    printf("  Hashes/core/s : %zu\n", average * SimdLanes());
}

int main(int argc, char* argv[])
{
    HashAlgorithm algorithm;
    size_t iterations = 1000000;

    //
    // If no arguments are passed, use the default
    // iterations will all algoritms
    //
    if (argc == 1)
    {
        for (size_t a = 0; a < SimdHashAlgorithmCount; a++)
        {
            PerformanceTests(SimdHashAlgorithms[a], iterations);
        }
    }
    else
    {
        algorithm = ParseHashAlgorithm(argv[1]);
        if (algorithm == HashUnknown)
        {
            fprintf(stderr, "Invalid hash algorithm\n");
            return(1);
        }

        if (argc > 2)
        {
            iterations = atoll(argv[2]);
        }

        PerformanceTests(algorithm, iterations);
    }
}
