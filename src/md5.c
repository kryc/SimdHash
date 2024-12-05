//
//  md5.c
//  SimdHash
//
//  Created by Gareth Evans on 15/01/2021.
//  Copyright © 2024 Gareth Evans. All rights reserved.
//

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>		// memset

#include <signal.h>

#include "simdhash.h"
#include "simdcommon.h"
#include "hashcommon.h"
#include "library.h"

static const uint32_t Md5InitialValues[] = {
    0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476
};

static const uint32_t Md5ShiftAmounts[] = {
    7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
    5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
    4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
    6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,
};

static const uint32_t Md5RoundConstants[] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

void
SimdMd5Init(
    SimdHashContext* Context)
{
    for (size_t i = 0; i < 4; i++)
    {
        store_simd(&Context->H[i].usimd, set1_epi32(Md5InitialValues[i]));
    }
    memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
    Context->HSize = MD5_H_COUNT;
    Context->HashSize = MD5_SIZE;
    Context->BufferSize = MD5_BUFFER_SIZE;
    Context->Lanes = SimdLanes();
    memset(Context->Offset, 0, sizeof(Context->Offset));
    memset(Context->BitLength, 0, sizeof(Context->BitLength));
    Context->Algorithm = HashAlgorithmMD5;
}

void
SimdMd5Transform(
    SimdHashContext* Context)
{
    simd_t f, g;
    
    simd_t aO, bO, cO, dO;
    simd_t a = aO = load_simd(&Context->H[0].usimd);
    simd_t b = bO = load_simd(&Context->H[1].usimd);
    simd_t c = cO = load_simd(&Context->H[2].usimd);
    simd_t d = dO = load_simd(&Context->H[3].usimd);

    //
    // Md5 compression function
    //
    for (size_t i = 0; i < 64; i++)
    {
        if (i < 16)
        {
            // F := (B and C) or ((not B) and D)
            f = SimdBitwiseChoiceWithControl(c, d, b);
            // g := i
            g = set1_epi32(i);
        }
        else if (i < 32)
        {
            // F := (D and B) or ((not D) and C)
            f = SimdBitwiseChoiceWithControl(b, c, d);
            // g := (5xi + 1) mod 16
            g = shift_mod2_epi32(add_epi32(mul_epu32(set1_epi32(i), set1_epi32(5)), set1_epi32(1)), LOG2_16);
        }
        else if (i < 48)
        {
            // F := B xor C xor D
            f = xor_simd(b, xor_simd(c, d));
            // g := (3×i + 5) mod 16
            g = shift_mod2_epi32(add_epi32(mul_epu32(set1_epi32(i), set1_epi32(3)), set1_epi32(5)), LOG2_16);
        }
        else //if (i < 64)
        {
            // F := C xor (B or (not D))
            f = xor_simd(c, or_simd(b, not_simd(d)));
            // g := (7×i) mod 16
            g = shift_mod2_epi32(mul_epu32(set1_epi32(i), set1_epi32(7)), LOG2_16);
        }
        
        SimdValue G, M;
        store_simd(&G.usimd, g);

        for (size_t j = 0; j < Context->Lanes; j++)
        {
            uint32_t index = G.epi32_u32[j];
            assert(index < 16);
            if (index >= 16)
            {
                printf("%u\n", index);
            }
            M.epi32_u32[j] = Context->Buffer[index].epi32_u32[j];
        }

        simd_t m = load_simd(&M.usimd);
        // raise(SIGTRAP);
        simd_t k = set1_epi32(Md5RoundConstants[i]);
        f = add_epi32(f, add_epi32(a, add_epi32(k, m)));
        a = d;
        d = c;
        c = b;
        b = add_epi32(b, rotl_epi32(f, Md5ShiftAmounts[i]));
    }
    
    //
    // Output to the hash state values
    //
    store_simd(&Context->H[0].usimd, add_epi32(aO, a));
    store_simd(&Context->H[1].usimd, add_epi32(bO, b));
    store_simd(&Context->H[2].usimd, add_epi32(cO, c));
    store_simd(&Context->H[3].usimd, add_epi32(dO, d));

    //
    // Reset the offset and buffer
    //
    memset(Context->Offset, 0, sizeof(Context->Offset));
    memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
}

static inline
void
SimdMd5AppendSize(
    SimdHashContext* Context)
/*++
 Appends the 1-bit and the message length to the hash buffer
 Also performs the additional Transform step if required
 --*/
{
    // Append the 1-bit to the buffer
    SimdHashUpdateInternal(
        Context,
        OneBitLengths,
        OneBits
    );

    // Remove the length of the bit from the total
    for (size_t lane = 0; lane < Context->Lanes; lane++)
    {
        Context->BitLength[lane] -= 8;
    }

    // Check if we have enough space for the
    // 64-bit length in all of the lanes
    const uint64_t needTransformMask = (1 << Context->Lanes) - 1;
    uint64_t needTransformLanes = 0;

    for (size_t lane = 0; lane < Context->Lanes; lane++)
    {
        if (Context->Offset[lane] >= 56 + 1)
        {
            needTransformLanes |= (1 << lane);
        }
    }

    if (needTransformLanes)
    {
        if (needTransformLanes == needTransformMask)
        {
            // If all lanes need to be transformed we just
            // do them all, easy!
            SimdMd5Transform(Context);
        }
        else
        {
            // Otherwise we need to trnasform and copy
            SimdHashContext contextcopy = *Context;
            SimdMd5Transform(&contextcopy);

            for (size_t lane = 0, lanemask = 1; lane < Context->Lanes; lane++, lanemask <<= 1)
            {
                if (needTransformLanes & lanemask)
                {
                    CopyContextLane(Context, &contextcopy, lane);
                }
            }
        }
    }

    for (size_t lane = 0; lane < Context->Lanes; lane++)
    {
        // Add the size to the last 64 bits
        SimdHashWriteBuffer64(
            Context,
            MD5_BUFFER_SIZE - sizeof(uint64_t),
            lane,
            Context->BitLength[lane]);
    }
}

void
SimdMd5Finalize(
    SimdHashContext* Context)
{
    // Add the message length
    SimdMd5AppendSize(Context);

    // Compute the final transformation
    SimdMd5Transform(Context);
}

void
SimdMd5FinalizeOptimized(
    SimdHashContext* Context)
{
    // Append the 1-bit to the buffer
    SimdHashUpdateInternal(
        Context,
        OneBitLengths,
        OneBits
    );

    // Add the message length
    for (size_t lane = 0; lane < Context->Lanes; lane++)
    {
        // Add the size to the last 64 bits
        SimdHashWriteBuffer64(
            Context,
            MD5_BUFFER_SIZE - sizeof(uint64_t),
            lane,
            Context->BitLength[lane] - 8
        );
    }

    // Perform the final transformation
    SimdMd5Transform(Context);
}