//
//  simdhash.c
//  SimdHash
//
//  Created by Gareth Evans on 17/01/2024.
//  Copyright © 2024 Gareth Evans. All rights reserved.
//

#include <alloca.h>
#include <string.h>
#include <openssl/md4.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <unicode/ucnv.h>
#include <unicode/ustring.h>

#include "simdhash.h"
#include "simdcommon.h"
#include "hashcommon.h"

const size_t
SimdLanes(
    void
)
{
    return (SIMD_WIDTH / 32);
}

const HashAlgorithm
ParseHashAlgorithm(
    const char* AlgorithmString
)
{
    if (strcmp(AlgorithmString, "md4") == 0 ||
        strcmp(AlgorithmString, "MD4") == 0)
    {
        return HashAlgorithmMD4;
    }
    else if (strcmp(AlgorithmString, "md5") == 0 ||
        strcmp(AlgorithmString, "MD5") == 0)
    {
        return HashAlgorithmMD5;
    }
    else if (strcmp(AlgorithmString, "sha1") == 0 ||
        strcmp(AlgorithmString, "SHA1") == 0)
    {
        return HashAlgorithmSHA1;
    }
    else if (strcmp(AlgorithmString, "sha256") == 0 ||
        strcmp(AlgorithmString, "SHA256") == 0)
    {
        return HashAlgorithmSHA256;
    }
    else if (strcmp(AlgorithmString, "sha384") == 0 ||
        strcmp(AlgorithmString, "SHA384") == 0)
    {
        return HashAlgorithmSHA384;
    }
    else if (strcmp(AlgorithmString, "sha512") == 0 ||
        strcmp(AlgorithmString, "SHA512") == 0)
    {
        return HashAlgorithmSHA512;
    }
    else if (strcmp(AlgorithmString, "ntlm") == 0 ||
        strcmp(AlgorithmString, "NTLM") == 0)
    {
        return HashAlgorithmNTLM;
    }
    return HashAlgorithmUndefined;
}

const char*
HashAlgorithmToString(
    const HashAlgorithm Algorithm
)
{
    switch (Algorithm)
    {
    case HashAlgorithmMD4:
        return "MD4";
    case HashAlgorithmMD5:
        return "MD5";
    case HashAlgorithmSHA1:
        return "SHA1";
    case HashAlgorithmSHA256:
        return "SHA256";
    case HashAlgorithmSHA384:
        return "SHA384";
    case HashAlgorithmSHA512:
        return "SHA512";
    case HashAlgorithmNTLM:
        return "NTLM";
    default:
        return "Unknown";
    }
}

const size_t
GetHashWidth(
    const HashAlgorithm Algorithm
)
{
    switch (Algorithm)
    {
    case HashAlgorithmMD4:
    case HashAlgorithmNTLM:
        return MD4_SIZE;
    case HashAlgorithmMD5:
        return MD5_SIZE;
    case HashAlgorithmSHA1:
        return SHA1_SIZE;
    case HashAlgorithmSHA256:
        return SHA256_SIZE;
    case HashAlgorithmSHA384:
        return SHA384_SIZE;
    case HashAlgorithmSHA512:
        return SHA512_SIZE;
    default:
        return (size_t)-1;
    }
}

const size_t
GetOptimizedLength(
	const HashAlgorithm Algorithm
)
{
    switch (Algorithm)
    {
    case HashAlgorithmMD4:
    // case HashAlgorithmNTLM:
        return MD4_OPTIMIZED_BUFFER_SIZE;
    case HashAlgorithmMD5:
        return MD5_OPTIMIZED_BUFFER_SIZE;
    case HashAlgorithmSHA1:
        return SHA1_OPTIMIZED_BUFFER_SIZE;
    case HashAlgorithmSHA256:
        return SHA256_OPTIMIZED_BUFFER_SIZE;
    default:
        return (size_t)-1;
    }
}

const HashAlgorithm
DetectHashAlgorithm(
    const size_t HashLength
)
{
    switch(HashLength)
    {
    case MD5_SIZE:
        return HashAlgorithmMD5;
    case SHA1_SIZE:
        return HashAlgorithmSHA1;
    case SHA256_SIZE:
        return HashAlgorithmSHA256;
    case SHA384_SIZE:
        return HashAlgorithmSHA384;
    case SHA512_SIZE:
        return HashAlgorithmSHA512;
    default:
        return HashAlgorithmUndefined;
    }
}

void SimdHashInit(
    SimdHashContext* Context,
    const HashAlgorithm Algorithm
)
{
    switch (Algorithm)
    {
    case HashAlgorithmMD4:
        SimdMd4Init(Context);
        break;
    case HashAlgorithmMD5:
        SimdMd5Init(Context);
        break;
    case HashAlgorithmSHA1:
        SimdSha1Init(Context);
        break;
    case HashAlgorithmSHA256:
        SimdSha256Init(Context);
        break;
    case HashAlgorithmSHA384:
        SimdSha384Init(Context);
        break;
    case HashAlgorithmSHA512:
        SimdSha512Init(Context);
        break;
    case HashAlgorithmUndefined:
        break;
    case HashAlgorithmNTLM:
        SimdMd4Init(Context);
        Context->Algorithm = HashAlgorithmNTLM;
        break;
    default:
        assert(false);
        break;
    }
}

void SimdHashTransform(
    SimdHashContext* Context
)
{
    switch (Context->Algorithm)
    {
    case HashAlgorithmMD4:
    case HashAlgorithmNTLM:
        SimdMd4Transform(Context);
        break;
    case HashAlgorithmMD5:
        SimdMd5Transform(Context);
        break;
    case HashAlgorithmSHA1:
        SimdSha1Transform(Context, false);
        break;
    case HashAlgorithmSHA256:
        SimdSha256Transform(Context, false);
        break;
    case HashAlgorithmUndefined:
        break;
    default:
        assert(false);
        break;
    }
}

void
CopyContextLane(
    SimdHashContext* Destination,
    const SimdHashContext* Source,
    const size_t Lane
)
{
    assert(Destination->Algorithm == Source->Algorithm);
    assert(Destination->BufferSize == Source->BufferSize);
    assert(Destination->Lanes == Source->Lanes);
    assert(Destination->HSize == Source->HSize);
    assert(Destination->HashSize == Source->HashSize);
    // Copy contents of H buffer
    for (size_t i = 0; i < Source->HSize; i++)
    {
        Destination->H[i].epi32_u32[Lane] = Source->H[i].epi32_u32[Lane];
    }
    // Copy the buffer contents
    for (size_t i = 0; i < Source->BufferSize / sizeof(uint32_t); i++)
    {
        Destination->Buffer[i].epi32_u32[Lane] = Source->Buffer[i].epi32_u32[Lane];
    }
    // Reset counters
    Destination->Offset[Lane] = Source->Offset[Lane];
    Destination->BitLength[Lane] = Source->BitLength[Lane];
}

void
SimdHashUpdateInternal(
    SimdHashContext* Context,
    const size_t Lengths[],
    const uint8_t* const Buffers[]
)
{
    size_t remainder[MAX_LANES];
    size_t haveRemainder = 0;
    SimdHashContext contextcopy;

    // Set the remainder values
    for (size_t lane = 0; lane < Context->Lanes; lane++)
    {
        remainder[lane] = Lengths[lane];
    }

    do
    {
        haveRemainder = 0;

        for (size_t lane = 0; lane < Context->Lanes; lane++)
        {
            if (remainder[lane])
            {
                const size_t offset = Lengths[lane] - remainder[lane];
                size_t toWrite = SimdHashUpdateLaneBuffer(
                    Context,
                    lane,
                    remainder[lane],
                    Buffers[lane] + offset
                );

                remainder[lane] = toWrite;
                if (toWrite != 0)
                {
                    haveRemainder++;
                }
            }
        }

        if (haveRemainder)
        {
            if (haveRemainder == Context->Lanes)
            {
                // Perform a transform across all lanes
                SimdHashTransform(Context);	
            }
            else
            {
                // Take a copy of the original context
                contextcopy = *Context;
                // Perform a transform across all lanes
                SimdHashTransform(&contextcopy);
                // Copy the new states for remainder lanes
                for (size_t lane = 0; lane < Context->Lanes; lane++)
                {
                    if (remainder[lane])
                    {
                        CopyContextLane(Context, &contextcopy, lane);
                    }
                }
            }
        }
    }while (haveRemainder);
}

void
SimdHashUpdateOptimized(
    SimdHashContext* Context,
    const size_t Lengths[],
    const uint8_t* Buffers[]
)
{
    assert(Context->Algorithm != HashAlgorithmNTLM);
    for (size_t lane = 0; lane < Context->Lanes; lane++)
    {
        assert(Lengths[lane] <= GetOptimizedLength(Context->Algorithm));
        (void) SimdHashUpdateLaneBuffer(
            Context,
            lane,
            Lengths[lane],
            Buffers[lane]
        );
    }
}

static void
SimdHashUpdateNTLM(
    SimdHashContext* Context,
    const size_t Lengths[],
    const uint8_t* const Buffers[]
)
{
    size_t newLengths[MAX_LANES];
    uint8_t* newBuffers[MAX_LANES];

    UErrorCode status = U_ZERO_ERROR;

    for (size_t i = 0; i < Context->Lanes; i++)
    {
        int32_t newLength;
        u_strFromUTF8Lenient(NULL, 0, &newLength, (const char*)Buffers[i], Lengths[i], &status);
        if (status != U_BUFFER_OVERFLOW_ERROR && status != U_STRING_NOT_TERMINATED_WARNING)
        { 
            fprintf(stderr, "Error: %s\n", u_errorName(status));
            // Fallback to hash the provided input
            newBuffers[i] = (uint8_t*)Buffers[i];
            newLengths[i] = Lengths[i];
            continue;
        }

        // Reset the status and allocate memory
        status = U_ZERO_ERROR;
        newBuffers[i] = (uint8_t*)alloca((newLength + 1) * sizeof(UChar));
        if (newBuffers[i] == NULL) {
            fprintf(stderr, "Memory allocation error\n");
            // Fallback to hash the provided input
            newBuffers[i] = (uint8_t*)Buffers[i];
            newLengths[i] = Lengths[i];
            continue;
        }

        // Convert UTF-8 to UTF-16
        u_strFromUTF8Lenient((UChar*)newBuffers[i], newLength + 1, NULL, (const char*)Buffers[i], Lengths[i], &status);
        if (U_FAILURE(status)) {
            fprintf(stderr, "Conversion error: %s\n", u_errorName(status));
            // Fallback to hash the provided input
            newBuffers[i] = (uint8_t*)Buffers[i];
            newLengths[i] = Lengths[i];
            continue;
        }

        newLengths[i] = (size_t)newLength * sizeof(UChar);
    }

    // Call the internal SimdHash update function
    SimdHashUpdateInternal(
        Context,
        newLengths,
        (const uint8_t**)newBuffers
    );
}

void
SimdHashUpdate(
    SimdHashContext* Context,
    const size_t Lengths[],
    const uint8_t* const Buffers[]
)
{
    switch (Context->Algorithm)
    {
    case HashAlgorithmMD4:
    case HashAlgorithmMD5:
    case HashAlgorithmSHA1:
    case HashAlgorithmSHA256:
        SimdHashUpdateInternal(
            Context,
            Lengths,
            Buffers
        );
        break;
    case HashAlgorithmNTLM:
        SimdHashUpdateNTLM(
            Context,
            Lengths,
            Buffers
        );
        break;
    case HashAlgorithmSHA384:
        SimdSha384Update(
            Context,
            Lengths,
            Buffers
        );
        break;
    case HashAlgorithmSHA512:
        SimdSha512Update(
            Context,
            Lengths,
            Buffers
        );
        break;
    case HashAlgorithmUndefined:
        break;
    }
}

void
SimdHashUpdateAll(
    SimdHashContext* Context,
    const size_t Length,
    const uint8_t* const Buffers[]
)
{
    size_t lengths[MAX_LANES];

    for (size_t lane = 0; lane < Context->Lanes; lane++)
    {
        lengths[lane] = Length;
    }

    return SimdHashUpdate(Context, lengths, Buffers);
}

void
SimdHashUpdateAllOptimized(
    SimdHashContext* Context,
    const size_t Length,
    const uint8_t* Buffers[]
)
{
    size_t lengths[MAX_LANES];

    for (size_t lane = 0; lane < Context->Lanes; lane++)
    {
        lengths[lane] = Length;
    }

    return SimdHashUpdateOptimized(Context, lengths, Buffers);
}

void
SimdHashGetHash(
    SimdHashContext* Context,
    uint8_t* HashBuffer,
    const size_t Lane
)
{
    uint32_t* nextBuffer;
    
    nextBuffer = (uint32_t*)HashBuffer;

    for (size_t i = 0; i < Context->HSize; i++)
    {
        nextBuffer[i] = Context->H[i].epi32_u32[Lane];
    }
}

void
SimdHashGetHashes2D(
    SimdHashContext* Context,
    uint8_t** HashBuffers
)
{
    for (size_t i = 0; i < Context->Lanes; i++)
    {
        SimdHashGetHash(Context, HashBuffers[i], i);
    }
}

static inline void
WriteSimdArrayToLinearBuffer(
    const SimdValue* Array,
    const size_t CountDwords,
    const uint8_t* HashBuffers
)
{
#if defined __AVX512F__ && defined AVXSCATTER
    for (size_t i = 0; i < CountDwords; i++)
    {
        __m512i h = _mm512_load_si512(&Array[i].usimd);
        __m512i index = _mm512_setr_epi32(
            (CountDwords * 0) + i,
            (CountDwords * 1) + i,
            (CountDwords * 2) + i,
            (CountDwords * 3) + i,
            (CountDwords * 4) + i,
            (CountDwords * 5) + i,
            (CountDwords * 6) + i,
            (CountDwords * 7) + i,
            (CountDwords * 8) + i,
            (CountDwords * 9) + i,
            (CountDwords * 10) + i,
            (CountDwords * 11) + i,
            (CountDwords * 12) + i,
            (CountDwords * 13) + i,
            (CountDwords * 14) + i,
            (CountDwords * 15) + i
        );
        _mm512_i32scatter_epi32(HashBuffers, index, h, 4);
    }
#else
    uint32_t* buffer = (uint32_t*) HashBuffers;
    for (size_t i = 0; i < CountDwords; i++)
    {
        for (size_t l = 0; l < SimdLanes(); l++)
        {
            buffer[(l * CountDwords) + i] = Array[i].epi32_u32[l];
        }
    }
#endif
}

void
SimdHashGetHashes(
    SimdHashContext* Context,
    const uint8_t* HashBuffers
)
{
    WriteSimdArrayToLinearBuffer(Context->H, Context->HSize, HashBuffers);
}

void
SimdHashExtendEntropyAndGetHashes(
    SimdHashContext* Context,
    uint8_t* HashBuffers,
    size_t CountDwords
)
{
    assert(CountDwords >= Context->HSize);

    // Check if we need to extend the entropy
    if (CountDwords == Context->HSize)
    {
        // No need to extend the entropy
        WriteSimdArrayToLinearBuffer(Context->H, Context->HSize, HashBuffers);
        return;
    }

    SimdValue buffer[CountDwords];

    for (size_t i = 0; i < Context->HSize; i++)
    {
        buffer[i].usimd = load_simd(&Context->H[i].usimd);
    }

    for (size_t i = Context->HSize; i < CountDwords; i++)
    {
        // s0 := (w[i-15] rightrotate  7) xor (w[i-15] rightrotate 18) xor (w[i-15] rightshift  3)
        // s1 := (w[i-2] rightrotate 17) xor (w[i-2] rightrotate 19) xor (w[i-2] rightshift 10)
        // w[i] := w[i-16] + s0 + w[i-7] + s1
        simd_t s0 = xor_simd(xor_simd(rotr_epi32(buffer[i - Context->HSize].usimd, 7), rotr_epi32(buffer[i - Context->HSize].usimd, 18)), srli_epi32(buffer[i - Context->HSize].usimd, 3));
        simd_t s1 = xor_simd(xor_simd(rotr_epi32(buffer[i - 2].usimd, 17), rotr_epi32(buffer[i - 2].usimd, 19)), srli_epi32(buffer[i - 2].usimd, 10));
        buffer[i].usimd = add_epi32(add_epi32(s0, s1), buffer[i - 3].usimd);
    }

    // Output to the hash buffers
    WriteSimdArrayToLinearBuffer(buffer, CountDwords, HashBuffers);
}

void
SimdHashFinalize(
    SimdHashContext* Context
)
{
    switch (Context->Algorithm)
    {
    case HashAlgorithmMD4:
    case HashAlgorithmNTLM:
        SimdMd4Finalize(Context);
        break;
    case HashAlgorithmMD5:
        SimdMd5Finalize(Context);
        break;
    case HashAlgorithmSHA1:
        SimdSha1Finalize(Context);
        break;
    case HashAlgorithmSHA256:
        SimdSha256Finalize(Context);
        break;
    case HashAlgorithmSHA384:
        SimdSha384Finalize(Context);
        break;
    case HashAlgorithmSHA512:
        SimdSha512Finalize(Context);
        break;
    case HashAlgorithmUndefined:
        break;
    default:
        assert(false);
        break;
    }
}

void
SimdHash(
    HashAlgorithm Algorithm,
    const size_t Lengths[],
    const uint8_t* const Buffers[],
    const uint8_t* HashBuffers
)
{
    switch(Algorithm)
    {
    case HashAlgorithmMD4:
    case HashAlgorithmMD5:
    case HashAlgorithmSHA1:
    case HashAlgorithmSHA256:
    case HashAlgorithmNTLM:
        {
            SimdHashContext ctx;
            SimdHashInit(&ctx, Algorithm);
            SimdHashUpdate(&ctx, Lengths, Buffers);
            SimdHashFinalize(&ctx);
            SimdHashGetHashes(&ctx, HashBuffers);
        }
        break;
    case HashAlgorithmSHA384:
    case HashAlgorithmSHA512:
        {
            const size_t hashWidth = GetHashWidth(Algorithm);
            for (size_t i = 0; i < SimdLanes(); i++)
            {
                const uint8_t* hash = &HashBuffers[i * hashWidth];
                SimdHashSingle(Algorithm, Lengths[i], Buffers[i], hash);
            }
        }
        break;
    case HashAlgorithmUndefined:
        break;
    }
}

void
SimdHashExtended(
    HashAlgorithm Algorithm,
    const size_t Lengths[],
    const uint8_t* const Buffers[],
    const uint8_t* HashBuffers,
    const size_t CountDwords
)
{
    switch(Algorithm)
    {
    case HashAlgorithmMD4:
    case HashAlgorithmMD5:
    case HashAlgorithmSHA1:
    case HashAlgorithmSHA256:
    case HashAlgorithmNTLM:
        {
            SimdHashContext ctx;
            SimdHashInit(&ctx, Algorithm);
            SimdHashUpdate(&ctx, Lengths, Buffers);
            SimdHashFinalize(&ctx);
            SimdHashExtendEntropyAndGetHashes(&ctx, (uint8_t*)HashBuffers, CountDwords);
        }
        break;
    case HashAlgorithmSHA384:
    case HashAlgorithmSHA512:
    case HashAlgorithmUndefined:
        break;
    }
}

void
SimdHashOptimized(
    HashAlgorithm Algorithm,
    const size_t Lengths[],
    const uint8_t* const Buffers[],
    const uint8_t* HashBuffers
)
{
    switch(Algorithm)
    {
    case HashAlgorithmMD4:
    case HashAlgorithmMD5:
    case HashAlgorithmSHA1:
    case HashAlgorithmSHA256:
    case HashAlgorithmNTLM:
        {
            SimdHashContext ctx;
            SimdHashInit(&ctx, Algorithm);
            SimdHashUpdate(&ctx, Lengths, Buffers);
            SimdHashFinalize(&ctx);
            SimdHashGetHashes(&ctx, HashBuffers);
        }
        break;
    case HashAlgorithmSHA384:
    case HashAlgorithmSHA512:
        {
            const size_t hashWidth = GetHashWidth(Algorithm);
            for (size_t i = 0; i < SimdLanes(); i++)
            {
                const uint8_t* hash = &HashBuffers[i * hashWidth];
                SimdHashSingle(Algorithm, Lengths[i], Buffers[i], hash);
            }
        }
        break;
    case HashAlgorithmUndefined:
        break;
    }
}

void
NTLMSingle(
    const uint8_t* const Buffer,
    const size_t Length,
    const uint8_t* HashBuffer
)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t newLength;
    uint8_t* buffer;

    u_strFromUTF8Lenient(NULL, 0, &newLength, (const char*)Buffer, Length, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && status != U_STRING_NOT_TERMINATED_WARNING)
    { 
        fprintf(stderr, "Error: %s\n", u_errorName(status));
        // Fallback to hash the provided input
        MD4(Buffer, Length, (uint8_t*) Buffer);
        return;
    }

    // Reset the status and allocate memory
    status = U_ZERO_ERROR;
    buffer = (uint8_t*)alloca((newLength + 1) * sizeof(UChar));
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        // Fallback to hash the provided input
        MD4(Buffer, Length, (uint8_t*) HashBuffer);
        return;
    }

    // Convert UTF-8 to UTF-16
    u_strFromUTF8Lenient((UChar*)buffer, newLength + 1, NULL, (const char*)Buffer, Length, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "Conversion error: %s\n", u_errorName(status));
        // Fallback to hash the provided input
        MD4(Buffer, Length, (uint8_t*) HashBuffer);
        return;
    }

    MD4(buffer, newLength * sizeof(UChar), (uint8_t*) HashBuffer);
}

void
SimdHashSingle(
    HashAlgorithm Algorithm,
    const size_t Length,
    const uint8_t* const Buffer,
    const uint8_t* HashBuffer
)
{
    switch(Algorithm)
    {
    case HashAlgorithmMD4:
        MD4(Buffer, Length, (uint8_t*) HashBuffer);
        break;
    case HashAlgorithmMD5:
        MD5(Buffer, Length, (uint8_t*) HashBuffer);
        break;
    case HashAlgorithmSHA1:
        SHA1(Buffer, Length, (uint8_t*) HashBuffer);
        break;
    case HashAlgorithmSHA256:
        SHA256(Buffer, Length, (uint8_t*) HashBuffer);
        break;
    case HashAlgorithmSHA384:
        SHA384(Buffer, Length, (uint8_t*) HashBuffer);
        break;
    case HashAlgorithmSHA512:
        SHA512(Buffer, Length, (uint8_t*) HashBuffer);
        break;
    case HashAlgorithmNTLM:
        NTLMSingle(Buffer, Length, (uint8_t*) HashBuffer);
        break;
    case HashAlgorithmUndefined:
        break;
    }
}

// rotate right
static inline uint32_t
ROR32(
    uint32_t value,
    int count
)
{
    return (value >> count) | (value << (32 - count));
}

void
SimdHashSingleExtended(
    HashAlgorithm Algorithm,
    const size_t Length,
    const uint8_t* const Buffer,
    const uint8_t* HashBuffer,
    const size_t CountDwords
)
{
    // Do the normal hash first
    SimdHashSingle(Algorithm, Length, Buffer, HashBuffer);
    // Then extend the entropy
    uint32_t* const hash = (uint32_t* const) HashBuffer;
    const size_t hashSize = GetHashWidth(Algorithm);
    const size_t hashDwords = hashSize / sizeof(uint32_t);
    for (size_t i = hashDwords; i < CountDwords; i++)
    {
        // s0 := (w[i-15] rightrotate  7) xor (w[i-15] rightrotate 18) xor (w[i-15] rightshift  3)
        // s1 := (w[i-2] rightrotate 17) xor (w[i-2] rightrotate 19) xor (w[i-2] rightshift 10)
        // w[i] := w[i-16] + s0 + w[i-7] + s1
        uint32_t s0 = ROR32(hash[i - hashDwords], 7) ^ ROR32(hash[i - hashDwords], 18) ^ (hash[i - hashDwords] >> 3);
        uint32_t s1 = ROR32(hash[i - 2], 17) ^ ROR32(hash[i - 2], 19) ^ (hash[i - 2] >> 10);
        hash[i] = hash[i - 3] + s0 + s1;
    }
}
