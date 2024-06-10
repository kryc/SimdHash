//
//  library.h
//  SimdHash
//
//  Created by Gareth Evans on 29/01/2024.
//  Copyright © 2021 Gareth Evans. All rights reserved.
//

#include <stdint.h>

#include "simdhash.h"

#ifndef library_h
#define library_h

static inline size_t
SimdHashWriteBuffer8(
    SimdHashContext* Context,
    const size_t Lane,
    const uint8_t Value
)
{
    const size_t length = Context->Offset[Lane];
    const size_t bufferIndex = length / 4;
    const size_t bufferOffset = length % 4;
    Context->Buffer[bufferIndex].epi32_u8[Lane][bufferOffset] = Value;
    return length + sizeof(uint8_t);
}

static inline size_t
SimdHashWriteBuffer32(
    SimdHashContext* Context,
    const size_t Lane,
    const uint32_t Value
)
{
    const size_t length = Context->Offset[Lane];
    const size_t bufferIndex = length / 4;
    Context->Buffer[bufferIndex].epi32_u32[Lane] = Value;
    return length + sizeof(uint32_t);
}

static inline size_t
SimdHashWriteBuffer64(
    SimdHashContext* Context,
    const size_t Lane,
    const uint64_t Value
)
{
    const size_t length = Context->Offset[Lane];
    const size_t bufferIndex = length / 4;
    Context->Buffer[bufferIndex].epi32_u32[Lane] = Value & 0xffffffff;
    Context->Buffer[bufferIndex + 1].epi32_u32[Lane] = Value >> 32;
    return length + sizeof(uint64_t);
}

#endif /* library_h */
