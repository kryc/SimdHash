//
//  main.c
//  SimdHash
//
//  Created by Gareth Evans on 25/08/2020.
//  Copyright © 2020 Gareth Evans. All rights reserved.
//

#include <stdio.h>
#include "sha2.h"

static const uint8_t target[] = {0xf0,0x39,0x3f,0xeb,0xe8,0xba,0xaa,0x55,0xe3,0x2f,0x7b,0xe2,0xa7,0xcc,0x18,0x0b,0xf3,0x4e,0x52,0x13,0x7d,0x99,0xe0,0x56,0xc8,0x17,0xa9,0xc0,0x7b,0x8f,0x23,0x9a};

int main(int argc, const char * argv[]) {
	ALIGN(16) SimdSha2Context ctx;
	ALIGN(16) SimdSha2SecondPreimageContext ctx2;
	size_t matching;
	
	SimdSha256Init(&ctx, argc-1);
	SimdSha256SecondPreimageInit(&ctx2, &ctx, target);
	matching = SimdSha256SecondPreimage(&ctx2, strlen(argv[1]), &argv[1]);
	
	printf("%zu: %s\n", matching, argv[matching+1]);
}
