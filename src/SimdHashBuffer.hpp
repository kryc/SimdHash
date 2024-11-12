//
//  SimdHashBuffer.hpp
//  SimdCrack
//
//  Created by Kryc on 20/01/2024.
//  Copyright © 2020 Kryc. All rights reserved.
//

#include <cinttypes>
#include <vector>

#include "simdhash.h"

#ifndef SimdHashBuffer_hpp
#define SimdHashBuffer_hpp

class SimdHashBuffer
{
public:
    SimdHashBuffer(const size_t Width, const size_t Count) :
    m_Width(Width), m_Count(Count)
    {
        m_Buffer.resize(m_Width * m_Count);
        m_Lengths.resize(m_Count);
        for (size_t i = 0; i < m_Count; i++)
        {
            m_BufferPointers.push_back(&m_Buffer[i * m_Width]);
        }
    }
    SimdHashBuffer(const size_t Width) :
        SimdHashBuffer(Width, SimdLanes()) {};
    const size_t GetWidth(void) const { return m_Width; };
    const size_t GetCount(void) const { return m_Count; };
    uint8_t* Buffer(void) { return &m_Buffer[0]; };
    uint8_t* Buffer(const size_t Index) { return m_BufferPointers[Index]; };
    uint8_t* operator[](const size_t Index) { return m_BufferPointers[Index]; };
    uint8_t** Buffers(void) { return &m_BufferPointers[0]; };
    const uint8_t** ConstBuffers(void) { return (const uint8_t**) &m_BufferPointers[0]; };
    const size_t* GetLengths(void) const { return &m_Lengths[0]; };
    void SetLength(const size_t Index, const size_t Length) { m_Lengths[Index] = Length; };
    const size_t GetLength(const size_t Index) { return m_Lengths[Index]; };
private:
    const size_t m_Width;
    const size_t m_Count;
    std::vector<uint8_t> m_Buffer;
    std::vector<uint8_t*> m_BufferPointers;
    std::vector<size_t> m_Lengths;
};

template <size_t Width, size_t Count=MAX_LANES>
class SimdHashBufferFixed
{
public:
    SimdHashBufferFixed(void)
    {
        for (size_t i = 0; i < Count; i++)
        {
            m_BufferPointers[i] = &m_Buffer[i * Width];
        }
    }
    const size_t GetWidth(void) const { return Width; };
    const size_t GetCount(void) const { return Count; };
    uint8_t* Buffer(void) { return &m_Buffer[0]; };
    uint8_t* Buffer(const size_t Index) { return m_BufferPointers[Index]; };
    uint8_t* operator[](const size_t Index) { return m_BufferPointers[Index]; };
    uint8_t** Buffers(void) { return &m_BufferPointers[0]; };
    const uint8_t** ConstBuffers(void) { return (const uint8_t**) &m_BufferPointers[0]; };
    const size_t* GetLengths(void) const { return &m_Lengths[0]; };
    const void Set(const size_t Index, const std::string& Value) {
        const size_t copylen = std::min<size_t>(Value.size(), Width);
        memcpy(m_BufferPointers[Index], &Value[0], copylen);
        m_Lengths[Index] = copylen;
    }
    void SetLength(const size_t Index, const size_t Length) { m_Lengths[Index] = Length; };
    const size_t GetLength(const size_t Index) { return m_Lengths[Index]; };
private:
    std::array<uint8_t, Count * Width> m_Buffer;
    std::array<size_t, Count> m_Lengths;
    std::array<uint8_t*, Count> m_BufferPointers;
};

#endif // SimdHashBuffer_hpp