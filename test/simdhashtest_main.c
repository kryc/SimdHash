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
	uint8_t  Md5Digest[MD5_SIZE];
	uint8_t  Sha1Digest[SHA1_SIZE];
	uint8_t  Sha256Digest[SHA256_SIZE];
} TestVector;

//
// Test vectors
// Source: https://www.di-mgt.com.au/sha_testvectors.html
//
static const TestVector SimdHashTestVectors[] = {
	{
		0,
		(uint8_t*)"",
		{0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04, 0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e},
		{0xda,0x39,0xa3,0xee,0x5e,0x6b,0x4b,0x0d,0x32,0x55,0xbf,0xef,0x95,0x60,0x18,0x90,0xaf,0xd8,0x07,0x09},
		{0xe3,0xb0,0xc4,0x42,0x98,0xfc,0x1c,0x14,0x9a,0xfb,0xf4,0xc8,0x99,0x6f,0xb9,0x24,0x27,0xae,0x41,0xe4,0x64,0x9b,0x93,0x4c,0xa4,0x95,0x99,0x1b,0x78,0x52,0xb8,0x55}
	},
	{
		3,
		(uint8_t*)"abc",
		{0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0, 0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72},
		{0xa9,0x99,0x3e,0x36,0x47,0x06,0x81,0x6a,0xba,0x3e,0x25,0x71,0x78,0x50,0xc2,0x6c,0x9c,0xd0,0xd8,0x9d},
		{0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad}
	},
	{
		6,
		(uint8_t*)"abcdef",
		{0xe8, 0x0b, 0x50, 0x17, 0x09, 0x89, 0x50, 0xfc, 0x58, 0xaa, 0xd8, 0x3c, 0x8c, 0x14, 0x97, 0x8e},
		{0x1f, 0x8a, 0xc1, 0x0f, 0x23, 0xc5, 0xb5, 0xbc, 0x11, 0x67, 0xbd, 0xa8, 0x4b, 0x83, 0x3e, 0x5c, 0x05, 0x7a, 0x77, 0xd2},
		{0xbe, 0xf5, 0x7e, 0xc7, 0xf5, 0x3a, 0x6d, 0x40, 0xbe, 0xb6, 0x40, 0xa7, 0x80, 0xa6, 0x39, 0xc8, 0x3b, 0xc2, 0x9a, 0xc8, 0xa9, 0x81, 0x6f, 0x1f, 0xc6, 0xc5, 0xc6, 0xdc, 0xd9, 0x3c, 0x47, 0x21}
	},
	{
		8,
		(uint8_t*)"abcdefgh",
		{0xe8, 0xdc, 0x40, 0x81, 0xb1, 0x34, 0x34, 0xb4, 0x51, 0x89, 0xa7, 0x20, 0xb7, 0x7b, 0x68, 0x18},
		{0x42, 0x5a, 0xf1, 0x2a, 0x07, 0x43, 0x50, 0x2b, 0x32, 0x2e, 0x93, 0xa0, 0x15, 0xbc, 0xf8, 0x68, 0xe3, 0x24, 0xd5, 0x6a},
		{0x9c, 0x56, 0xcc, 0x51, 0xb3, 0x74, 0xc3, 0xba, 0x18, 0x92, 0x10, 0xd5, 0xb6, 0xd4, 0xbf, 0x57, 0x79, 0x0d, 0x35, 0x1c, 0x96, 0xc4, 0x7c, 0x02, 0x19, 0x0e, 0xcf, 0x1e, 0x43, 0x06, 0x35, 0xab}
	},
	{
		55,
		(uint8_t*)"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnop",
		{0x28, 0x07, 0xd6, 0x52, 0xab, 0x02, 0xf7, 0x36, 0x11, 0xc9, 0x94, 0xe5, 0xd5, 0xac, 0x92, 0x21},
		{0x47, 0xb1, 0x72, 0x81, 0x07, 0x95, 0x69, 0x9f, 0xe7, 0x39, 0x19, 0x7d, 0x1a, 0x1f, 0x59, 0x60, 0x70, 0x02, 0x42, 0xf1},
		{0xaa, 0x35, 0x3e, 0x00, 0x9e, 0xdb, 0xae, 0xbf, 0xc6, 0xe4, 0x94, 0xc8, 0xd8, 0x47, 0x69, 0x68, 0x96, 0xcb, 0x8b, 0x39, 0x8e, 0x01, 0x73, 0xa4, 0xb5, 0xc1, 0xb6, 0x36, 0x29, 0x2d, 0x87, 0xc7}
	}/*,
	{
		56,
		(uint8_t*)"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
		{0x82, 0x15, 0xef, 0x07, 0x96, 0xa2, 0x0b, 0xca, 0xaa, 0xe1, 0x16, 0xd3, 0x87, 0x6c, 0x66, 0x4a},
		{0x84,0x98,0x3e,0x44,0x1c,0x3b,0xd2,0x6e,0xba,0xae,0x4a,0xa1,0xf9,0x51,0x29,0xe5,0xe5,0x46,0x70,0xf1},
		{0x24,0x8d,0x6a,0x61,0xd2,0x06,0x38,0xb8,0xe5,0xc0,0x26,0x93,0x0c,0x3e,0x60,0x39,0xa3,0x3c,0xe4,0x59,0x64,0xff,0x21,0x67,0xf6,0xec,0xed,0xd4,0x19,0xdb,0x06,0xc1}
	},
	{
		112,
		(uint8_t*)"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
		{0x03, 0xdd, 0x88, 0x07, 0xa9, 0x31, 0x75, 0xfb, 0x06, 0x2d, 0xfb, 0x55, 0xdc, 0x7d, 0x35, 0x9c},
		{0xa4,0x9b,0x24,0x46,0xa0,0x2c,0x64,0x5b,0xf4,0x19,0xf9,0x95,0xb6,0x70,0x91,0x25,0x3a,0x04,0xa2,0x59},
		{0xcf,0x5b,0x16,0xa7,0x78,0xaf,0x83,0x80,0x03,0x6c,0xe5,0x9e,0x7b,0x04,0x92,0x37,0x0b,0x24,0x9b,0x11,0xe8,0xf0,0x7a,0x51,0xaf,0xac,0x45,0x03,0x7a,0xfe,0xe9,0xd1}
	}*/
};

static const size_t VectorCount = sizeof(SimdHashTestVectors)/sizeof(TestVector);

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
		snprintf(nextOut, 3, "%02x", Buffer[i]);
		nextOut += 2;
	}
	
	return true;
}

static void
GetTestVector(
	const size_t Index,
	const HashAlgorithm Algorithm,
	uint8_t** PreImage,
	size_t* PreImageLength,
	uint8_t** Digest,
	size_t* DigestLength
)
{
	*PreImage = NULL;
	*PreImageLength = 0;
	*Digest = NULL;
	*DigestLength = 0;

	if (Index >= VectorCount)
	{
		return;
	}

	*PreImage = SimdHashTestVectors[Index].PreImage;
	*PreImageLength = SimdHashTestVectors[Index].Length;

	switch (Algorithm)
	{
	case HashMd5:
		*Digest = (uint8_t*)SimdHashTestVectors[Index].Md5Digest;
		*DigestLength = MD5_SIZE;
		break;
	case HashSha1:
		*Digest = (uint8_t*)SimdHashTestVectors[Index].Sha1Digest;
		*DigestLength = SHA1_SIZE;
		break;
	case HashSha256:
		*Digest = (uint8_t*)SimdHashTestVectors[Index].Sha256Digest;
		*DigestLength = SHA256_SIZE;
		break;
	default:
		return;
	}
}

static bool
FunctionalityTests(
	const HashAlgorithm Algorithm
)
/*++
	This function performs the basic functionality
	tests. It uses the known digest values to ensure
	that the algorithms are providing correct results	
--*/
{
	SimdHashContext context;
	uint8_t* buffers[SimdLanes()];
	uint8_t hash[SHA256_SIZE];
	char hex[SHA256_SIZE * 2 + 1];
	uint8_t hashes[SimdLanes() * MAX_BUFFER_SIZE];
	bool fail;
	uint8_t* preimage;
	size_t preimageSize;
	uint8_t* expectedDigest;
	size_t digestSize;

	const size_t VectorLanesMax = SimdLanes() > VectorCount ? VectorCount : SimdLanes();

	memset(hex, 0, sizeof(hex));

	fail = false;

	for (size_t c = 0; c < VectorCount; c++)
	{
		// Get the test vector values for the algorithm
		GetTestVector(c, Algorithm, &preimage, &preimageSize, &expectedDigest, &digestSize);
		
		// Set the buffer pointers
		for (size_t i = 0; i < SimdLanes(); i++)
		{
			buffers[i] = preimage;
		}
		
		// Perform the hash
		SimdHashInit(&context, Algorithm);
		SimdHashUpdateAll(&context, preimageSize, (const uint8_t**)buffers);
		SimdHashFinalize(&context);
		SimdHashGetHash(&context, hash, 0);
		SimdHashGetHashes(&context, hashes);

		// Test if GetHashes results match
		for (size_t i = 0; i < SimdLanes(); i++)
		{
			if (memcmp(hash, hashes + (i * digestSize), digestSize) != 0)
			{
				fail = true;
				printf("[!] %6s:   SimdGetHashes Fail\n", HashAlgorithmToString(Algorithm));
			}
		}
		
		ToHex(hex, sizeof(hex), expectedDigest, digestSize);
		printf("[+] Expected: %s\n", hex);

		ToHex(hex, sizeof(hex), hash, digestSize);
		if (memcmp(&hash[0], expectedDigest, digestSize) != 0)
		{
			fail = true;
			printf("[!] %6s:   %s\n", HashAlgorithmToString(Algorithm), hex);
		}
		else
		{
			printf("[+] %6s:   %s\n", HashAlgorithmToString(Algorithm), hex);
		}
	}

	printf("\n[+] Mixed length tests\n");

	size_t lengths[SimdLanes()];
	
	for (size_t c = 0; c < VectorLanesMax; c++)
	{
		// Get the test vector values for the algorithm
		GetTestVector(c, Algorithm, &preimage, &preimageSize, &expectedDigest, &digestSize);
		// We only use Length and PreImage here
		lengths[c] = preimageSize;
		buffers[c] = preimage;
	}

	SimdHashInit(&context, Algorithm);
	SimdHashSetLaneCount(&context, VectorLanesMax);
	SimdHashUpdate(&context, lengths, (const uint8_t**)buffers);
	SimdHashFinalize(&context);

	for (size_t c = 0; c < VectorLanesMax; c++)
	{
		// Get the test vector values for the algorithm
		GetTestVector(c, Algorithm, &preimage, &preimageSize, &expectedDigest, &digestSize);

		SimdHashGetHash(&context, hash, c);
		ToHex(hex, sizeof(hex), expectedDigest, digestSize);
		printf("[+] Expected: %s\n", hex);
		ToHex(hex, sizeof(hex), hash, digestSize);
		if (memcmp(&hash[0], expectedDigest, digestSize) != 0)
		{
			fail = true;
			printf("[!] %6s:   %s\n", HashAlgorithmToString(Algorithm), hex);
		}
		else
		{
			printf("[+] %6s:   %s\n", HashAlgorithmToString(Algorithm), hex);
		}
	}

	printf("\n[+] Multiple update tests\n");
	
	for (size_t c = 0; c < VectorLanesMax; c++)
	{
		// Skip the zero-length vector
		GetTestVector(c + 1, Algorithm, &preimage, &preimageSize, &expectedDigest, &digestSize);
		lengths[c] = 1;
		buffers[c] = preimage;
	}

	SimdHashInit(&context, Algorithm);
	SimdHashSetLaneCount(&context, VectorLanesMax - 1);
	SimdHashUpdate(&context, lengths, (const uint8_t**)buffers);

	// Do remaining bytes
	for (size_t c = 0; c < VectorLanesMax; c++)
	{
		GetTestVector(c + 1, Algorithm, &preimage, &preimageSize, &expectedDigest, &digestSize);
		lengths[c] = preimageSize - 1;
		buffers[c] = preimage + 1;
	}

	SimdHashUpdate(&context, lengths, (const uint8_t**)buffers);
	SimdHashFinalize(&context);

	for (size_t c = 0; c < VectorLanesMax; c++)
	{
		// Get the test vector values for the algorithm
		GetTestVector(c + 1, Algorithm, &preimage, &preimageSize, &expectedDigest, &digestSize);

		SimdHashGetHash(&context, hash, c);
		ToHex(hex, sizeof(hex), expectedDigest, digestSize);
		printf("[+] Expected: %s\n", hex);
		ToHex(hex, sizeof(hex), hash, digestSize);
		if (memcmp(&hash[0], expectedDigest, digestSize) != 0)
		{
			fail = true;
			printf("[!] %6s:   %s\n", HashAlgorithmToString(Algorithm), hex);
		}
		else
		{
			printf("[+] %6s:   %s\n", HashAlgorithmToString(Algorithm), hex);
		}
	}

	if (fail)
	{
		printf("\n[!] %s Functional test(s) failed!\n", HashAlgorithmToString(Algorithm));
	}
	else
	{
		printf("\n[+] %s Functional tests passed!\n", HashAlgorithmToString(Algorithm));
	}
	return !fail;
}

int main(int argc, char* argv[])
{
	HashAlgorithm algorithm;

    //
    // If no arguments are passed, test all algorithms
    //
    if (argc == 1)
    {
        for (size_t a = 0; a < SimdHashAlgorithmCount; a++)
        {
            FunctionalityTests(SimdHashAlgorithms[a]);
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

        FunctionalityTests(algorithm);
    }
}
