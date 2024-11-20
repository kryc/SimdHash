//
//  md5.c
//  SimdHash
//
//  Created by Gareth Evans on 15/01/2021.
//  Copyright © 2024 Gareth Evans. All rights reserved.
//

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h> // memset

#include <signal.h>

#include "simdhash.h"
#include "simdcommon.h"
#include "hashcommon.h"
#include "library.h"

static const uint32_t Md4InitialValues[] = {
    0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};

void SimdMd4Init(
    SimdHashContext *Context)
{
    for (size_t i = 0; i < 4; i++)
    {
        store_simd(&Context->H[i].usimd, set1_epi32(Md4InitialValues[i]));
    }
#ifdef OPTIMIZED
    // We never need to clear the last two DWORDS as
    // they will only contain the bit count
    memset(Context - Buffer, 0x00, sizeof(SimdValue) * (MD5_OPTIMIZED_BUFFER_SIZE_DWORDS));
#else
    memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
#endif
    Context->HSize = MD4_H_COUNT;
    Context->HashSize = MD4_SIZE;
    Context->BufferSize = MD4_BUFFER_SIZE;
    Context->Lanes = SimdLanes();
    memset(Context->Offset, 0, sizeof(Context->Offset));
    memset(Context->BitLength, 0, sizeof(Context->BitLength));
    Context->Algorithm = HashAlgorithmMD4;
}

static inline simd_t
FF(
    simd_t A,
    simd_t B,
    simd_t C,
    simd_t D,
    simd_t X,
    uint8_t S)
{
    // f(X,Y,Z)  =  XY v not(X)Z
    // Let [A B C D i s] denote the operation
    //      A = (A + f(B,C,D) + X[i]) <<< s
    simd_t f = SimdBitwiseChoiceWithControl(C, D, B);
    simd_t ff1 = add_epi32(A, f);
    simd_t ff2 = add_epi32(ff1, X);
    return rotl_epi32(ff2, S);
}

static inline simd_t
GG(
    simd_t A,
    simd_t B,
    simd_t C,
    simd_t D,
    simd_t X,
    uint8_t S)
{
    // g(X,Y,Z)  =  XY v XZ v YZ
    // Let [A B C D i s] denote the operation
    //      A = (A + g(B,C,D) + X[i] + 5A827999) <<< s
    simd_t gg1 = add_epi32(A, SimdBitwiseMajority(B, C, D));
    simd_t gg2 = add_epi32(gg1, X);
    simd_t gg3 = add_epi32(gg2, set1_epi32(0x5A827999));
    return rotl_epi32(gg3, S);
}

static inline simd_t
HH(
    simd_t A,
    simd_t B,
    simd_t C,
    simd_t D,
    simd_t X,
    uint8_t S)
{
    // h(X,Y,Z)  =  X xor Y xor Z
    // Let [A B C D i s] denote the operation
    //      A = (A + h(B,C,D) + X[i] + 6ED9EBA1) <<< s
    simd_t h = xor_simd(xor_simd(B, C), D);
    simd_t hh1 = add_epi32(A, h);
    simd_t hh2 = add_epi32(hh1, X);
    simd_t hh3 = add_epi32(hh2, set1_epi32(0x6ED9EBA1));
    return rotl_epi32(hh3, S);
}

static inline void
SimdMd4Transform(
    SimdHashContext *Context)
{
    simd_t aO, bO, cO, dO;
    simd_t a = aO = load_simd(&Context->H[0].usimd);
    simd_t b = bO = load_simd(&Context->H[1].usimd);
    simd_t c = cO = load_simd(&Context->H[2].usimd);
    simd_t d = dO = load_simd(&Context->H[3].usimd);
    simd_t x[16];

    // Inirialize x with the buffer contents
    for (size_t i = 0; i < 16; i++)
    {
        x[i] = load_simd(&Context->Buffer[i].usimd);
    }

    //
    // Round 1
    //
    // [A B C D 0 3]
    a = FF(a, b, c, d, x[0], 3);
    // [D A B C 1 7]
    d = FF(d, a, b, c, x[1], 7);
    // [C D A B 2 11]
    c = FF(c, d, a, b, x[2], 11);
    // [B C D A 3 19]
    b = FF(b, c, d, a, x[3], 19);
    // [A B C D 4 3]
    a = FF(a, b, c, d, x[4], 3);
    // [D A B C 5 7]
    d = FF(d, a, b, c, x[5], 7);
    // [C D A B 6 11]
    c = FF(c, d, a, b, x[6], 11);
    // [B C D A 7 19]
    b = FF(b, c, d, a, x[7], 19);
    // [A B C D 8 3]
    a = FF(a, b, c, d, x[8], 3);
    // [D A B C 9 7]
    d = FF(d, a, b, c, x[9], 7);
    // [C D A B 10 11]
    c = FF(c, d, a, b, x[10], 11);
    // [B C D A 11 19]
    b = FF(b, c, d, a, x[11], 19);
    // [A B C D 12 3]
    a = FF(a, b, c, d, x[12], 3);
    // [D A B C 13 7]
    d = FF(d, a, b, c, x[13], 7);
    // [C D A B 14 11]
    c = FF(c, d, a, b, x[14], 11);
    // [B C D A 15 19]
    b = FF(b, c, d, a, x[15], 19);

    //
    // Round 2
    //
    // [A B C D 0  3]
    a = GG(a, b, c, d, x[0], 3);
    // [D A B C 4  5]
    d = GG(d, a, b, c, x[4], 5);
    // [C D A B 8  9]
    c = GG(c, d, a, b, x[8], 9);
    // [B C D A 12 13]
    b = GG(b, c, d, a, x[12], 13);
    // [A B C D 1  3]
    a = GG(a, b, c, d, x[1], 3);
    // [D A B C 5  5]
    d = GG(d, a, b, c, x[5], 5);
    // [C D A B 9  9]
    c = GG(c, d, a, b, x[9], 9);
    // [B C D A 13 13]
    b = GG(b, c, d, a, x[13], 13);
    // [A B C D 2  3]
    a = GG(a, b, c, d, x[2], 3);
    // [D A B C 6  5]
    d = GG(d, a, b, c, x[6], 5);
    // [C D A B 10 9]
    c = GG(c, d, a, b, x[10], 9);
    // [B C D A 14 13]
    b = GG(b, c, d, a, x[14], 13);
    // [A B C D 3  3]
    a = GG(a, b, c, d, x[3], 3);
    // [D A B C 7  5]
    d = GG(d, a, b, c, x[7], 5);
    // [C D A B 11 9]
    c = GG(c, d, a, b, x[11], 9);
    // [B C D A 15 13]
    b = GG(b, c, d, a, x[15], 13);

    //
    // Round 3
    //
    // [A B C D 0  3]
    a = HH(a, b, c, d, x[0], 3);
    // [D A B C 8  9]
    d = HH(d, a, b, c, x[8], 9);
    // [C D A B 4  11]
    c = HH(c, d, a, b, x[4], 11);
    // [B C D A 12 15]
    b = HH(b, c, d, a, x[12], 15);
    // [A B C D 2  3]
    a = HH(a, b, c, d, x[2], 3);
    // [D A B C 10 9]
    d = HH(d, a, b, c, x[10], 9);
    // [C D A B 6  11]
    c = HH(c, d, a, b, x[6], 11);
    // [B C D A 14 15]
    b = HH(b, c, d, a, x[14], 15);
    // [A B C D 1  3]
    a = HH(a, b, c, d, x[1], 3);
    // [D A B C 9  9]
    d = HH(d, a, b, c, x[9], 9);
    // [C D A B 5  11]
    c = HH(c, d, a, b, x[5], 11);
    // [B C D A 13 15]
    b = HH(b, c, d, a, x[13], 15);
    // [A B C D 3  3]
    a = HH(a, b, c, d, x[3], 3);
    // [D A B C 11 9]
    d = HH(d, a, b, c, x[11], 9);
    // [C D A B 7  11]
    c = HH(c, d, a, b, x[7], 11);
    // [B C D A 15 15]
    b = HH(b, c, d, a, x[15], 15);

    //
    // Output to the hash state values
    //
    store_simd(&Context->H[0].usimd, add_epi32(aO, a));
    store_simd(&Context->H[1].usimd, add_epi32(bO, b));
    store_simd(&Context->H[2].usimd, add_epi32(cO, c));
    store_simd(&Context->H[3].usimd, add_epi32(dO, d));
}

static inline void
SimdMd4AppendSize(
    SimdHashContext *Context)
/*++
 Appends the 1-bit and the message length to the hash buffer
 Also performs the additional Transform step if required
 --*/
{
    for (size_t lane = 0; lane < Context->Lanes; lane++)
    {
        //
        // Write the 1 bit
        //
        const size_t offset = Context->Offset[lane];
        SimdHashWriteBuffer8(Context, offset, lane, 0x80);

        //
        // Check if we need to do another round
        //
        // if (Context->Offset >= 56)
        // {
        // 	SimdMd4Transform(Context);
        // 	memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
        // }

        // Bump the used buffer length to add the size to
        // the last 64 bits
        SimdHashWriteBuffer64(
            Context,
            MD4_BUFFER_SIZE - sizeof(uint64_t),
            lane,
            Context->BitLength[lane]);
    }
}

void SimdMd4Finalize(
    SimdHashContext *Context)
{
    //
    // Add the message length
    //
    SimdMd4AppendSize(Context);

    //
    // Compute the final transformation
    //
    SimdMd4Transform(Context);
}