//
//  main.c
//  SimdHash
//
//  Created by Gareth Evans on 25/08/2020.
//  Copyright © 2020 Gareth Evans. All rights reserved.
//

#include <stdio.h>
#include "sha2.h"

int main(int argc, const char * argv[]) {
	ALIGN(16) SimdSha2Context ctx;
	
	SimdSha256Init(&ctx, argc-1);
	SimdSha256Update(&ctx, strlen(argv[1]), &argv[1]);
	SimdSha256Finalize(&ctx);
}
