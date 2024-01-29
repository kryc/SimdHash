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
	uint8_t  Sha256Digest[SHA256_SIZE];
	uint8_t  Sha1Digest[SHA1_SIZE];
	uint8_t  Md5Digest[MD5_SIZE];
} TestVector;

//
// Test vectors
// Source: https://www.di-mgt.com.au/sha_testvectors.html
//
static const TestVector g_ShaTestVectors[] = {
	{
		0,
		(uint8_t*)"",
		{0xe3,0xb0,0xc4,0x42,0x98,0xfc,0x1c,0x14,0x9a,0xfb,0xf4,0xc8,0x99,0x6f,0xb9,0x24,0x27,0xae,0x41,0xe4,0x64,0x9b,0x93,0x4c,0xa4,0x95,0x99,0x1b,0x78,0x52,0xb8,0x55},
		{0xda,0x39,0xa3,0xee,0x5e,0x6b,0x4b,0x0d,0x32,0x55,0xbf,0xef,0x95,0x60,0x18,0x90,0xaf,0xd8,0x07,0x09},
		{0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04, 0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e}
	},
	{
		3,
		(uint8_t*)"abc",
		{0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad},
		{0xa9,0x99,0x3e,0x36,0x47,0x06,0x81,0x6a,0xba,0x3e,0x25,0x71,0x78,0x50,0xc2,0x6c,0x9c,0xd0,0xd8,0x9d},
		{0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0, 0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72}
	},
	{
		6,
		(uint8_t*)"abcdef",
		{0xbe, 0xf5, 0x7e, 0xc7, 0xf5, 0x3a, 0x6d, 0x40, 0xbe, 0xb6, 0x40, 0xa7, 0x80, 0xa6, 0x39, 0xc8, 0x3b, 0xc2, 0x9a, 0xc8, 0xa9, 0x81, 0x6f, 0x1f, 0xc6, 0xc5, 0xc6, 0xdc, 0xd9, 0x3c, 0x47, 0x21},
		{0x1f, 0x8a, 0xc1, 0x0f, 0x23, 0xc5, 0xb5, 0xbc, 0x11, 0x67, 0xbd, 0xa8, 0x4b, 0x83, 0x3e, 0x5c, 0x05, 0x7a, 0x77, 0xd2},
		{0xe8, 0x0b, 0x50, 0x17, 0x09, 0x89, 0x50, 0xfc, 0x58, 0xaa, 0xd8, 0x3c, 0x8c, 0x14, 0x97, 0x8e}
	},
	{
		55,
		(uint8_t*)"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnop",
		{0xaa, 0x35, 0x3e, 0x00, 0x9e, 0xdb, 0xae, 0xbf, 0xc6, 0xe4, 0x94, 0xc8, 0xd8, 0x47, 0x69, 0x68, 0x96, 0xcb, 0x8b, 0x39, 0x8e, 0x01, 0x73, 0xa4, 0xb5, 0xc1, 0xb6, 0x36, 0x29, 0x2d, 0x87, 0xc7},
		{0x47, 0xb1, 0x72, 0x81, 0x07, 0x95, 0x69, 0x9f, 0xe7, 0x39, 0x19, 0x7d, 0x1a, 0x1f, 0x59, 0x60, 0x70, 0x02, 0x42, 0xf1},
		{0x28, 0x07, 0xd6, 0x52, 0xab, 0x02, 0xf7, 0x36, 0x11, 0xc9, 0x94, 0xe5, 0xd5, 0xac, 0x92, 0x21}
	}/*,
	{
		56,
		(uint8_t*)"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
		{0x24,0x8d,0x6a,0x61,0xd2,0x06,0x38,0xb8,0xe5,0xc0,0x26,0x93,0x0c,0x3e,0x60,0x39,0xa3,0x3c,0xe4,0x59,0x64,0xff,0x21,0x67,0xf6,0xec,0xed,0xd4,0x19,0xdb,0x06,0xc1},
		{0x84,0x98,0x3e,0x44,0x1c,0x3b,0xd2,0x6e,0xba,0xae,0x4a,0xa1,0xf9,0x51,0x29,0xe5,0xe5,0x46,0x70,0xf1},
		{0x82, 0x15, 0xef, 0x07, 0x96, 0xa2, 0x0b, 0xca, 0xaa, 0xe1, 0x16, 0xd3, 0x87, 0x6c, 0x66, 0x4a}
	},
	{
		112,
		(uint8_t*)"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
		{0xcf,0x5b,0x16,0xa7,0x78,0xaf,0x83,0x80,0x03,0x6c,0xe5,0x9e,0x7b,0x04,0x92,0x37,0x0b,0x24,0x9b,0x11,0xe8,0xf0,0x7a,0x51,0xaf,0xac,0x45,0x03,0x7a,0xfe,0xe9,0xd1},
		{0xa4,0x9b,0x24,0x46,0xa0,0x2c,0x64,0x5b,0xf4,0x19,0xf9,0x95,0xb6,0x70,0x91,0x25,0x3a,0x04,0xa2,0x59},
		{0x03, 0xdd, 0x88, 0x07, 0xa9, 0x31, 0x75, 0xfb, 0x06, 0x2d, 0xfb, 0x55, 0xdc, 0x7d, 0x35, 0x9c}
	}*/
};

static bool
ToHex(char* Out, size_t OutLength, const uint8_t* Buffer, size_t BufferLength)
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
	SimdHashContext ctx;
	// Sha2Context sha2ctx;
	uint8_t* buffers[SimdLanes()];
	uint8_t hash[SHA256_SIZE];
	char hex[SHA256_SIZE * 2 + 1];
	bool sha256fail, sha1fail, md5fail;

	memset(hex, 0, sizeof(hex));
	
	sha256fail = false;
	sha1fail = false;
	md5fail = false;

	for (size_t c = 0; c < sizeof(g_ShaTestVectors)/sizeof(*g_ShaTestVectors); c++)
	{
		printf("[+] Vector %zu\n", c);
		
		for (size_t i = 0; i < SimdLanes(); i++)
		{
			buffers[i] = (uint8_t*)g_ShaTestVectors[c].PreImage;
		}
		
		SimdSha256Init(&ctx);
		SimdHashUpdateAll(&ctx, g_ShaTestVectors[c].Length, (const uint8_t**)buffers);
		SimdSha256Finalize(&ctx);
		SimdHashGetHash(&ctx, hash, 0);
		
		ToHex(hex, sizeof(hex), &g_ShaTestVectors[c].Sha256Digest[0], SHA256_SIZE);
		printf("[+] Expected: %s\n", hex);

		ToHex(hex, sizeof(hex), hash, SHA256_SIZE);
		if (memcmp(&hash[0], &g_ShaTestVectors[c].Sha256Digest[0], SHA256_SIZE) != 0)
		{
			sha256fail = true;
			printf("[!] SHA256:   %s\n", hex);
		}
		else
		{
			printf("[+] SHA256:   %s\n", hex);
		}
		
		SimdSha1Init(&ctx);
		SimdHashUpdateAll(&ctx, g_ShaTestVectors[c].Length, (const uint8_t**)buffers);
		SimdSha1Finalize(&ctx);
		SimdHashGetHash(&ctx, hash, 0);
		
		ToHex(hex, sizeof(hex), &g_ShaTestVectors[c].Sha1Digest[0], SHA1_SIZE);
		printf("[+] Expected: %s\n", hex);

		ToHex(hex, sizeof(hex), hash, SHA1_SIZE);
		
		if (memcmp(&hash[0], &g_ShaTestVectors[c].Sha1Digest[0], SHA1_SIZE) != 0)
		{
			sha1fail = true;
			printf("[!] SHA1:     %s\n", hex);
		}
		else
		{
			printf("[+] SHA1:     %s\n", hex);
		}

		SimdMd5Init(&ctx);
		SimdHashUpdateAll(&ctx, g_ShaTestVectors[c].Length, (const uint8_t**)buffers);
		SimdMd5Finalize(&ctx);
		SimdHashGetHash(&ctx, hash, 0);
		
		ToHex(hex, sizeof(hex), &g_ShaTestVectors[c].Md5Digest[0], MD5_SIZE);
		printf("[+] Expected: %s\n", hex);

		ToHex(hex, sizeof(hex), hash, MD5_SIZE);
		
		if (memcmp(&hash[0], &g_ShaTestVectors[c].Md5Digest[0], MD5_SIZE) != 0)
		{
			md5fail = true;
			printf("[!] MD5:      %s\n", hex);
		}
		else
		{
			printf("[+] MD5:      %s\n", hex);
		}
	}

	printf("\n[+] Mixed length tests\n");

	size_t lengths[SimdLanes()];
	
	for (size_t c = 0; c < sizeof(g_ShaTestVectors)/sizeof(*g_ShaTestVectors); c++)
	{
		lengths[c] = g_ShaTestVectors[c].Length;
		buffers[c] = g_ShaTestVectors[c].PreImage;
	}

	SimdSha256Init(&ctx);
	SimdHashUpdate(&ctx, lengths, (const uint8_t**)buffers);
	SimdSha256Finalize(&ctx);

	for (size_t c = 0; c < sizeof(g_ShaTestVectors)/sizeof(*g_ShaTestVectors); c++)
	{
		SimdHashGetHash(&ctx, hash, c);
		ToHex(hex, sizeof(hex), &g_ShaTestVectors[c].Sha256Digest[0], SHA256_SIZE);
		printf("[+] Expected: %s\n", hex);
		ToHex(hex, sizeof(hex), hash, SHA256_SIZE);
		if (memcmp(&hash[0], &g_ShaTestVectors[c].Sha256Digest[0], SHA256_SIZE) != 0)
		{
			sha256fail = true;
			printf("[!] SHA256:   %s\n", hex);
		}
		else
		{
			printf("[+] SHA256:   %s\n", hex);
		}
	}

	SimdSha1Init(&ctx);
	SimdHashUpdate(&ctx, lengths, (const uint8_t**)buffers);
	SimdSha1Finalize(&ctx);

	for (size_t c = 0; c < sizeof(g_ShaTestVectors)/sizeof(*g_ShaTestVectors); c++)
	{
		SimdHashGetHash(&ctx, hash, c);
		ToHex(hex, sizeof(hex), &g_ShaTestVectors[c].Sha1Digest[0], SHA1_SIZE);
		printf("[+] Expected: %s\n", hex);
		ToHex(hex, sizeof(hex), hash, SHA1_SIZE);
		if (memcmp(&hash[0], &g_ShaTestVectors[c].Sha1Digest[0], SHA1_SIZE) != 0)
		{
			sha1fail = true;
			printf("[!] SHA1:     %s\n", hex);
		}
		else
		{
			printf("[+] SHA1:     %s\n", hex);
		}
	}

	SimdMd5Init(&ctx);
	SimdHashUpdate(&ctx, lengths, (const uint8_t**)buffers);
	SimdMd5Finalize(&ctx);

	for (size_t c = 0; c < sizeof(g_ShaTestVectors)/sizeof(*g_ShaTestVectors); c++)
	{
		SimdHashGetHash(&ctx, hash, c);
		ToHex(hex, sizeof(hex), &g_ShaTestVectors[c].Md5Digest[0], MD5_SIZE);
		printf("[+] Expected: %s\n", hex);
		ToHex(hex, sizeof(hex), hash, MD5_SIZE);
		if (memcmp(&hash[0], &g_ShaTestVectors[c].Md5Digest[0], MD5_SIZE) != 0)
		{
			md5fail = true;
			printf("[!] MD5:      %s\n", hex);
		}
		else
		{
			printf("[+] MD5:      %s\n", hex);
		}
	}

	if (sha256fail)
	{
		printf("\n[!] SHA256 Functional test(s) failed!\n");
	}
	else
	{
		printf("\n[+] SHA256 Functional tests passed!\n");
	}
	
	if (sha1fail)
	{
		printf("[!] SHA1 Functional test(s) failed!\n");
	}
	else
	{
		printf("[+] SHA1 Functional tests passed!\n");
	}
	
	if (md5fail)
	{
		printf("[!] MD5 Functional test(s) failed!\n");
	}
	else
	{
		printf("[+] MD5 Functional tests passed!\n");
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
	SimdHashContext sha256ctx;
	uint8_t* buffers[SimdLanes()];
	struct timespec begin;
	long elapsed;
	size_t hashesPerSec, fastest, slowest, average;
	static const size_t numTests = 1000000;

	//
	// Initialize the buffers
	//
	for (size_t i = 0; i < SimdLanes(); i++)
	{
		buffers[i] = (uint8_t*)g_ShaTestVectors[0].PreImage;
	}

	average = 0;
	fastest = 0;
	slowest = SIZE_MAX;

	//
	// SHA256
	// Perform tests, capturing statistics
	//
	for (size_t i = 0; i < numTests; i++)
	{
		begin = timer_start();
		SimdSha256Init(&sha256ctx);
		SimdHashUpdateAll(&sha256ctx, g_ShaTestVectors[0].Length, (const uint8_t**)buffers);
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
	printf("SIMD SHA256 Performance Tests over %zu iterations\n", numTests);
	printf("Fastest (8h/s): %zu\n", fastest);
	printf("Slowest (8h/s): %zu\n", slowest);
	printf("Average (8h/s): %zu\n", average);
	printf("Hashes/core/s : %zu\n", average * 8);

	average = 0;
	fastest = 0;
	slowest = SIZE_MAX;

	//
	// SHA1
	// Perform tests, capturing statistics
	//
	for (size_t i = 0; i < numTests; i++)
	{
		begin = timer_start();
		SimdSha1Init(&sha256ctx);
		SimdHashUpdateAll(&sha256ctx, g_ShaTestVectors[0].Length, (const uint8_t**)buffers);
		SimdSha1Finalize(&sha256ctx);
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
	printf("SIMD SHA1 Performance Tests over %zu iterations\n", numTests);
	printf("Fastest (8h/s): %zu\n", fastest);
	printf("Slowest (8h/s): %zu\n", slowest);
	printf("Average (8h/s): %zu\n", average);
	printf("Hashes/core/s : %zu\n", average * 8);
}

int main(int argc, char* argv[])
{
	FunctionalityTests();
	// PerformanceTests();
}
