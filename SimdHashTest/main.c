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

int main(int arc, char* argv[])
{
	SimdSha2Context sha256ctx;
	uint8_t* buffers[SIMD_COUNT];
	uint8_t hash[SHA256_SIZE];
	
	for (size_t i = 0; i < SIMD_COUNT; i++)
	{
		buffers[i] = (uint8_t*)g_TestData;
	}
	
	SimdSha256Init(&sha256ctx, SIMD_COUNT);
	SimdSha256Update(&sha256ctx, strlen(g_TestData), (const uint8_t**)buffers);
	SimdSha256Finalize(&sha256ctx);
	
	SimdSha256GetHash(&sha256ctx, hash, 0);
	
	if (memcmp(&hash[0], &g_TestExpected[0], sizeof(g_TestExpected)) == 0)
	{
		printf("Test passed\n");
	}
	else
	{
		printf("Test failed\n");
	}
}
