//
//  SimdHashBuffer.hpp
//  SimdCrack
//
//  Created by Kryc on 20/01/2024.
//  Copyright © 2020 Kryc. All rights reserved.
//

#include <string.h>

#include <cinttypes>
#include <cstring>
#include <string>
#include <vector>
#include <span>
#include <string>
#include <string_view>

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
        m_BufferPointers.reserve(m_Count);
        for (size_t i = 0; i < m_Count; i++)
        {
            m_BufferPointers.push_back(&m_Buffer[i * m_Width]);
        }
        m_Span = std::span<uint8_t>(m_Buffer);
    }
    SimdHashBuffer(const size_t Width) :
        SimdHashBuffer(Width, SimdLanes()) {};
    const size_t GetWidth(void) const { return m_Width; };
    const size_t GetCount(void) const { return m_Count; };
    uint8_t* Buffer(void) { return &m_Buffer[0]; };
    uint8_t* Buffer(const size_t Index) const { return m_BufferPointers[Index]; };
    uint8_t* operator[](const size_t Index) const { return m_BufferPointers[Index]; };
    uint8_t** Buffers(void) { return &m_BufferPointers[0]; };
    const uint8_t** ConstBuffers(void) const { return (const uint8_t**) &m_BufferPointers[0]; };
    const size_t* GetLengths(void) const { return &m_Lengths[0]; };
    const void Set(const size_t Index, const std::string_view Value) {
        const size_t copylen = std::min<size_t>(Value.size(), m_Width);
        auto span = m_Span.subspan(Index * m_Width, m_Width);
        std::memcpy(span.data(), Value.data(), copylen);
        m_Lengths[Index] = copylen;
    }
    const std::span<const uint8_t> Get(const size_t Index) const {
        return m_Span.subspan(Index * m_Width, m_Width);
    }
    void SetLength(const size_t Index, const size_t Length) { m_Lengths[Index] = Length; };
    const size_t GetLength(const size_t Index) const { return m_Lengths[Index]; };
    std::span<uint8_t> GetSpan(void) const { return m_Span; }
    const std::string_view GetStringView(const size_t Index) const {
        auto span = Get(Index);
        return std::string_view((const char*)span.data(), m_Lengths[Index]);
    }
    const std::string GetString(const size_t Index) const {
        return std::string(GetStringView(Index));
    }
    void Clear(void) {
        m_Buffer.assign(m_Buffer.size(), 0);
        m_Lengths.assign(m_Lengths.size(), 0);  
    }
private:
    const size_t m_Width;
    const size_t m_Count;
    std::span<uint8_t> m_Span;
    std::vector<uint8_t> m_Buffer;
    std::vector<uint8_t*> m_BufferPointers;
    std::vector<size_t> m_Lengths;
};

template <size_t Width, size_t Count=MAX_LANES>
class SimdHashBufferFixed
{
public:
    SimdHashBufferFixed(void) : m_Span(m_Buffer)
    {
        for (size_t i = 0; i < Count; i++)
        {
            m_BufferPointers[i] = &m_Buffer[i * Width];
        }
    }
    const size_t GetWidth(void) const { return Width; };
    const size_t GetCount(void) const { return Count; };
    uint8_t* Buffer(void) const { return &m_Buffer[0]; };
    uint8_t* Buffer(const size_t Index) const { return m_BufferPointers[Index]; };
    uint8_t* operator[](const size_t Index) const { return m_BufferPointers[Index]; };
    uint8_t** Buffers(void) const { return &m_BufferPointers[0]; };
    const uint8_t** ConstBuffers(void) const { return (const uint8_t**) &m_BufferPointers[0]; };
    const size_t* GetLengths(void) const { return &m_Lengths[0]; };
    const void Set(const size_t Index, const std::string_view Value) {
        const size_t copylen = std::min<size_t>(Value.size(), Width);
        auto span = m_Span.subspan(Index * Width, Width);
        std::memcpy(span.data(), Value.data(), copylen);
        m_Lengths[Index] = copylen;
    }
    const std::span<const uint8_t> Get(const size_t Index) const {
        return m_Span.subspan(Index * Width, m_Lengths[Index]);
    }
    std::span<char> GetCharWriteSpan(const size_t Index) const {
#pragma clang unsafe_buffer_usage begin
        auto span = std::span<char>((char*)m_Buffer.data(), Count * Width);
#pragma clang unsafe_buffer_usage end
        return span.subspan(Index * Width, Width);
    }
    void SetLength(const size_t Index, const size_t Length) { m_Lengths[Index] = Length; };
    const size_t GetLength(const size_t Index) const { return m_Lengths[Index]; };
    std::span<uint8_t> GetSpan(void) const { return m_Span; }
    const std::string_view GetStringView(const size_t Index) const {
        auto span = Get(Index);
        return std::string_view((const char*)span.data(), m_Lengths[Index]);
    }
    const std::string GetString(const size_t Index) const {
        return std::string(GetStringView(Index));
    }
    void Clear(void) { m_Buffer.fill(0); m_Lengths.fill(0); }
private:
    std::array<uint8_t, Count * Width> m_Buffer;
    std::span<uint8_t, Count * Width> m_Span;
    std::array<size_t, Count> m_Lengths;
    std::array<uint8_t*, Count> m_BufferPointers;
};

#endif // SimdHashBuffer_hpp