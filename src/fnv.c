//
//  fnv.c
//  SimdHash
//
//  FNV-1 and FNV-1a hash (32-bit and 64-bit variants)
//  SIMD-parallel implementation
//

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "simdhash.h"
#include "simdcommon.h"
#include "library.h"

// FNV-1/1a 32-bit constants
#define FNV32_OFFSET_BASIS 0x811c9dc5
#define FNV32_PRIME        0x01000193

// FNV-1/1a 64-bit constants
#define FNV64_OFFSET_BASIS 0xcbf29ce484222325ULL
#define FNV64_PRIME        0x00000100000001B3ULL

// ============================================================
// FNV-1 32-bit
// ============================================================

void
SimdFnv1_32Init(
    SimdHashContext* Context)
{
    store_simd(&Context->H[0].usimd, set1_epi32(FNV32_OFFSET_BASIS));
    memset(&Context->H[1], 0, sizeof(SimdValue) * (MAX_H_COUNT - 1));
    memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
    Context->HSize = 1;  // single 32-bit hash word
    Context->HashSize = 4;
    Context->BufferSize = 0;  // not used — FNV processes inline
    Context->Lanes = SimdLanes();
    memset(Context->Offset, 0, sizeof(Context->Offset));
    memset(Context->BitLength, 0, sizeof(Context->BitLength));
    Context->Algorithm = HashAlgorithmFNV1_32;
}

void
SimdFnv1_32Update(
    SimdHashContext* Context,
    const size_t Lengths[],
    const uint8_t* const Buffers[])
{
    simd_t hash = load_simd(&Context->H[0].usimd);
    const simd_t prime = set1_epi32(FNV32_PRIME);

    // Find the max length across all lanes
    size_t maxLen = 0;
    for (size_t i = 0; i < Context->Lanes; i++)
    {
        if (Lengths[i] > maxLen) maxLen = Lengths[i];
    }

    // Process byte-by-byte across all lanes in parallel
    for (size_t pos = 0; pos < maxLen; pos++)
    {
        SimdValue byte_val __attribute__((__aligned__(VALUE_ALIGN)));
        SimdValue mask_val __attribute__((__aligned__(VALUE_ALIGN)));
        for (size_t lane = 0; lane < Context->Lanes; lane++)
        {
            byte_val.epi32_u32[lane] = (pos < Lengths[lane]) ? Buffers[lane][pos] : 0;
            mask_val.epi32_u32[lane] = (pos < Lengths[lane]) ? 0xFFFFFFFF : 0;
        }

        simd_t b = load_simd(&byte_val.usimd);
        simd_t mask = load_simd(&mask_val.usimd);
        // FNV-1: hash = (hash * prime) XOR byte
        simd_t new_hash = xor_simd(mul_epu32(hash, prime), b);
        // Blend: only update lanes that still have data
        hash = or_simd(and_simd(mask, new_hash), andnot_simd(mask, hash));
    }

    store_simd(&Context->H[0].usimd, hash);

    for (size_t lane = 0; lane < Context->Lanes; lane++)
    {
        Context->BitLength[lane] += Lengths[lane] * 8;
    }
}

void
SimdFnv1_32Finalize(
    SimdHashContext* Context)
{
    // FNV has no finalization step — hash state is the final digest
    (void)Context;
}

// ============================================================
// FNV-1a 32-bit
// ============================================================

void
SimdFnv1a_32Init(
    SimdHashContext* Context)
{
    store_simd(&Context->H[0].usimd, set1_epi32(FNV32_OFFSET_BASIS));
    memset(&Context->H[1], 0, sizeof(SimdValue) * (MAX_H_COUNT - 1));
    memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
    Context->HSize = 1;
    Context->HashSize = 4;
    Context->BufferSize = 0;
    Context->Lanes = SimdLanes();
    memset(Context->Offset, 0, sizeof(Context->Offset));
    memset(Context->BitLength, 0, sizeof(Context->BitLength));
    Context->Algorithm = HashAlgorithmFNV1a_32;
}

void
SimdFnv1a_32Update(
    SimdHashContext* Context,
    const size_t Lengths[],
    const uint8_t* const Buffers[])
{
    simd_t hash = load_simd(&Context->H[0].usimd);
    const simd_t prime = set1_epi32(FNV32_PRIME);

    size_t maxLen = 0;
    for (size_t i = 0; i < Context->Lanes; i++)
    {
        if (Lengths[i] > maxLen) maxLen = Lengths[i];
    }

    for (size_t pos = 0; pos < maxLen; pos++)
    {
        SimdValue byte_val __attribute__((__aligned__(VALUE_ALIGN)));
        SimdValue mask_val __attribute__((__aligned__(VALUE_ALIGN)));
        for (size_t lane = 0; lane < Context->Lanes; lane++)
        {
            byte_val.epi32_u32[lane] = (pos < Lengths[lane]) ? Buffers[lane][pos] : 0;
            mask_val.epi32_u32[lane] = (pos < Lengths[lane]) ? 0xFFFFFFFF : 0;
        }

        simd_t b = load_simd(&byte_val.usimd);
        simd_t mask = load_simd(&mask_val.usimd);
        // FNV-1a: hash = (hash XOR byte) * prime
        simd_t new_hash = mul_epu32(xor_simd(hash, b), prime);
        // Blend: only update lanes that still have data
        hash = or_simd(and_simd(mask, new_hash), andnot_simd(mask, hash));
    }

    store_simd(&Context->H[0].usimd, hash);

    for (size_t lane = 0; lane < Context->Lanes; lane++)
    {
        Context->BitLength[lane] += Lengths[lane] * 8;
    }
}

void
SimdFnv1a_32Finalize(
    SimdHashContext* Context)
{
    (void)Context;
}

// ============================================================
// FNV-1 64-bit
// ============================================================

void
SimdFnv1_64Init(
    SimdHashContext* Context)
{
    // Store 64-bit offset basis: low 32 bits in H[0], high 32 bits in H[1]
    store_simd(&Context->H[0].usimd, set1_epi32((uint32_t)(FNV64_OFFSET_BASIS & 0xFFFFFFFF)));
    store_simd(&Context->H[1].usimd, set1_epi32((uint32_t)(FNV64_OFFSET_BASIS >> 32)));
    memset(&Context->H[2], 0, sizeof(SimdValue) * (MAX_H_COUNT - 2));
    memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
    Context->HSize = 2;  // two 32-bit words = 64-bit hash
    Context->HashSize = 8;
    Context->BufferSize = 0;
    Context->Lanes = SimdLanes();
    memset(Context->Offset, 0, sizeof(Context->Offset));
    memset(Context->BitLength, 0, sizeof(Context->BitLength));
    Context->Algorithm = HashAlgorithmFNV1_64;
}

/*
 * 64-bit multiply using 32-bit SIMD lanes:
 *   result_lo = (a_lo * b_lo)                      [lower 32 bits of full 64-bit product]
 *   result_hi = (a_hi * b_lo) + (a_lo * b_hi) + carry_from_lo
 *
 * Since we only need the lower 64 bits of the 128-bit product, and
 * FNV64_PRIME's high 32 bits are 0x00000100, this simplifies.
 */
static inline void
SimdMul64(
    simd_t* lo, simd_t* hi,
    const simd_t a_lo, const simd_t a_hi,
    const simd_t b_lo, const simd_t b_hi)
{
    // Full 64-bit product (lower 64 bits only):
    // lo = low32(a_lo * b_lo)
    // hi = low32(a_hi * b_lo) + low32(a_lo * b_hi) + high32(a_lo * b_lo)

    // a_lo * b_lo: need all 64 bits
    simd_t prod_ll = mul_epu32(a_lo, b_lo);
    simd_t prod_ll_hi = srli_epi32(mul_epu32(
        or_simd(and_simd(a_lo, set1_epi32(0xFFFF0000)),
                srli_epi32(a_lo, 16)),
        b_lo), 0); // This approach doesn't work cleanly...

    // Simpler approach: use the fact that mul_epu32 gives us low 32 bits per lane
    // For the high 32 bits of a_lo*b_lo, we need the 64-bit product
    // mul_epu32 already does a_lo*b_lo with low32 result per lane
    *lo = mul_epu32(a_lo, b_lo);

    // For hi: a_hi*b_lo + a_lo*b_hi + carry
    // We can't easily get carry from the 32-bit mul_epu32...
    // Use decomposition: a_lo = a_lo_lo | (a_lo_hi << 16)
    //   a_lo * b_lo = a_lo_lo*b_lo + (a_lo_hi*b_lo) << 16
    // But this is getting complex. Let's use a simpler scalar approach for 64-bit.

    // Actually, the simplest correct approach for 64-bit FNV in 32-bit SIMD:
    // Process per-lane scalarly for the multiply, keep everything else SIMD.
    // FNV-1 64-bit is not as commonly needed for cracking workloads.
    (void)prod_ll;
    (void)prod_ll_hi;
    (void)b_hi;

    // Fallback: scalar multiply per lane
    SimdValue a_lo_v __attribute__((__aligned__(VALUE_ALIGN)));
    SimdValue a_hi_v __attribute__((__aligned__(VALUE_ALIGN)));
    SimdValue r_lo_v __attribute__((__aligned__(VALUE_ALIGN)));
    SimdValue r_hi_v __attribute__((__aligned__(VALUE_ALIGN)));

    store_simd(&a_lo_v.usimd, a_lo);
    store_simd(&a_hi_v.usimd, a_hi);

    for (size_t i = 0; i < SimdLanes(); i++)
    {
        uint64_t a = ((uint64_t)a_hi_v.epi32_u32[i] << 32) | a_lo_v.epi32_u32[i];
        uint64_t result = a * FNV64_PRIME;
        r_lo_v.epi32_u32[i] = (uint32_t)(result & 0xFFFFFFFF);
        r_hi_v.epi32_u32[i] = (uint32_t)(result >> 32);
    }

    *lo = load_simd(&r_lo_v.usimd);
    *hi = load_simd(&r_hi_v.usimd);
}

void
SimdFnv1_64Update(
    SimdHashContext* Context,
    const size_t Lengths[],
    const uint8_t* const Buffers[])
{
    simd_t hash_lo = load_simd(&Context->H[0].usimd);
    simd_t hash_hi = load_simd(&Context->H[1].usimd);
    const simd_t prime_lo = set1_epi32((uint32_t)(FNV64_PRIME & 0xFFFFFFFF));
    const simd_t prime_hi = set1_epi32((uint32_t)(FNV64_PRIME >> 32));

    size_t maxLen = 0;
    for (size_t i = 0; i < Context->Lanes; i++)
    {
        if (Lengths[i] > maxLen) maxLen = Lengths[i];
    }

    for (size_t pos = 0; pos < maxLen; pos++)
    {
        SimdValue byte_val __attribute__((__aligned__(VALUE_ALIGN)));
        SimdValue mask_val __attribute__((__aligned__(VALUE_ALIGN)));
        for (size_t lane = 0; lane < Context->Lanes; lane++)
        {
            byte_val.epi32_u32[lane] = (pos < Lengths[lane]) ? Buffers[lane][pos] : 0;
            mask_val.epi32_u32[lane] = (pos < Lengths[lane]) ? 0xFFFFFFFF : 0;
        }
        simd_t b = load_simd(&byte_val.usimd);
        simd_t mask = load_simd(&mask_val.usimd);

        // FNV-1: hash = (hash * prime) XOR byte
        simd_t new_lo, new_hi;
        SimdMul64(&new_lo, &new_hi, hash_lo, hash_hi, prime_lo, prime_hi);

        // Blend: only update lanes that still have data
        hash_lo = or_simd(and_simd(mask, xor_simd(new_lo, b)), andnot_simd(mask, hash_lo));
        hash_hi = or_simd(and_simd(mask, new_hi), andnot_simd(mask, hash_hi));
    }

    store_simd(&Context->H[0].usimd, hash_lo);
    store_simd(&Context->H[1].usimd, hash_hi);

    for (size_t lane = 0; lane < Context->Lanes; lane++)
    {
        Context->BitLength[lane] += Lengths[lane] * 8;
    }
}

void
SimdFnv1_64Finalize(
    SimdHashContext* Context)
{
    (void)Context;
}

// ============================================================
// FNV-1a 64-bit
// ============================================================

void
SimdFnv1a_64Init(
    SimdHashContext* Context)
{
    store_simd(&Context->H[0].usimd, set1_epi32((uint32_t)(FNV64_OFFSET_BASIS & 0xFFFFFFFF)));
    store_simd(&Context->H[1].usimd, set1_epi32((uint32_t)(FNV64_OFFSET_BASIS >> 32)));
    memset(&Context->H[2], 0, sizeof(SimdValue) * (MAX_H_COUNT - 2));
    memset(Context->Buffer, 0x00, sizeof(Context->Buffer));
    Context->HSize = 2;
    Context->HashSize = 8;
    Context->BufferSize = 0;
    Context->Lanes = SimdLanes();
    memset(Context->Offset, 0, sizeof(Context->Offset));
    memset(Context->BitLength, 0, sizeof(Context->BitLength));
    Context->Algorithm = HashAlgorithmFNV1a_64;
}

void
SimdFnv1a_64Update(
    SimdHashContext* Context,
    const size_t Lengths[],
    const uint8_t* const Buffers[])
{
    simd_t hash_lo = load_simd(&Context->H[0].usimd);
    simd_t hash_hi = load_simd(&Context->H[1].usimd);
    const simd_t prime_lo = set1_epi32((uint32_t)(FNV64_PRIME & 0xFFFFFFFF));
    const simd_t prime_hi = set1_epi32((uint32_t)(FNV64_PRIME >> 32));

    size_t maxLen = 0;
    for (size_t i = 0; i < Context->Lanes; i++)
    {
        if (Lengths[i] > maxLen) maxLen = Lengths[i];
    }

    for (size_t pos = 0; pos < maxLen; pos++)
    {
        SimdValue byte_val __attribute__((__aligned__(VALUE_ALIGN)));
        SimdValue mask_val __attribute__((__aligned__(VALUE_ALIGN)));
        for (size_t lane = 0; lane < Context->Lanes; lane++)
        {
            byte_val.epi32_u32[lane] = (pos < Lengths[lane]) ? Buffers[lane][pos] : 0;
            mask_val.epi32_u32[lane] = (pos < Lengths[lane]) ? 0xFFFFFFFF : 0;
        }
        simd_t b = load_simd(&byte_val.usimd);
        simd_t mask = load_simd(&mask_val.usimd);

        // FNV-1a: hash = (hash XOR byte) * prime
        simd_t xored_lo = xor_simd(hash_lo, b);
        // XOR only affects lo — byte value fits in 32 bits

        simd_t new_lo, new_hi;
        SimdMul64(&new_lo, &new_hi, xored_lo, hash_hi, prime_lo, prime_hi);

        // Blend: only update lanes that still have data
        hash_lo = or_simd(and_simd(mask, new_lo), andnot_simd(mask, hash_lo));
        hash_hi = or_simd(and_simd(mask, new_hi), andnot_simd(mask, hash_hi));
    }

    store_simd(&Context->H[0].usimd, hash_lo);
    store_simd(&Context->H[1].usimd, hash_hi);

    for (size_t lane = 0; lane < Context->Lanes; lane++)
    {
        Context->BitLength[lane] += Lengths[lane] * 8;
    }
}

void
SimdFnv1a_64Finalize(
    SimdHashContext* Context)
{
    (void)Context;
}

// ============================================================
// Scalar single-hash implementations for reference/test
// ============================================================

void
Fnv1_32Single(
    const uint8_t* Buffer,
    const size_t Length,
    uint8_t* HashBuffer)
{
    uint32_t hash = FNV32_OFFSET_BASIS;
    for (size_t i = 0; i < Length; i++)
    {
        hash *= FNV32_PRIME;
        hash ^= Buffer[i];
    }
    *(uint32_t*)HashBuffer = hash;
}

void
Fnv1a_32Single(
    const uint8_t* Buffer,
    const size_t Length,
    uint8_t* HashBuffer)
{
    uint32_t hash = FNV32_OFFSET_BASIS;
    for (size_t i = 0; i < Length; i++)
    {
        hash ^= Buffer[i];
        hash *= FNV32_PRIME;
    }
    *(uint32_t*)HashBuffer = hash;
}

void
Fnv1_64Single(
    const uint8_t* Buffer,
    const size_t Length,
    uint8_t* HashBuffer)
{
    uint64_t hash = FNV64_OFFSET_BASIS;
    for (size_t i = 0; i < Length; i++)
    {
        hash *= FNV64_PRIME;
        hash ^= Buffer[i];
    }
    *(uint64_t*)HashBuffer = hash;
}

void
Fnv1a_64Single(
    const uint8_t* Buffer,
    const size_t Length,
    uint8_t* HashBuffer)
{
    uint64_t hash = FNV64_OFFSET_BASIS;
    for (size_t i = 0; i < Length; i++)
    {
        hash ^= Buffer[i];
        hash *= FNV64_PRIME;
    }
    *(uint64_t*)HashBuffer = hash;
}
