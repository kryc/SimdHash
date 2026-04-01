//
//  SimdHash.hpp
//  SimdHash
//
//  Created by Kryc on 02/04/2025.
//  Copyright © 2025 Kryc. All rights reserved.
//

// C++ convenience Wrapper around some SimdHash functionality

#ifndef SimdHash_hpp
#define SimdHash_hpp

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <iostream>

#include "simdhash.h"

namespace simdhash
{

namespace
{
    static inline void
    CheckImpl(
        const bool Condition,
        const char* Message
    )
    {
        if (!Condition)
        {
            std::cerr << "Check failed: " << Message << std::endl;
            std::abort();
        }
    }
}

inline std::array<HashAlgorithm, SimdHashCryptoAlgorithmCount>
SimdHashCryptoAlgorithms{
    HashAlgorithmMD4,
    HashAlgorithmMD5,
    HashAlgorithmSHA1,
    HashAlgorithmSHA256,
    HashAlgorithmSHA384,
    HashAlgorithmSHA512,
    HashAlgorithmNTLM
};

inline std::array<HashAlgorithm, SimdHashOtherAlgorithmCount>
SimdHashOtherAlgorithms{
    HashAlgorithmFNV1_32,
    HashAlgorithmFNV1a_32,
    HashAlgorithmFNV1_64,
    HashAlgorithmFNV1a_64
};

inline std::array<HashAlgorithm, SimdHashAlgorithmCount>
SimdHashAlgorithms{
    HashAlgorithmMD4,
    HashAlgorithmMD5,
    HashAlgorithmSHA1,
    HashAlgorithmSHA256,
    HashAlgorithmSHA384,
    HashAlgorithmSHA512,
    HashAlgorithmNTLM,
    HashAlgorithmFNV1_32,
    HashAlgorithmFNV1a_32,
    HashAlgorithmFNV1_64,
    HashAlgorithmFNV1a_64
};

template<typename T>
static inline void
SimdHashSingle(
    const HashAlgorithm Algorithm,
    const T Input,
    std::span<uint8_t> Output
)
{
    const size_t requiredSize = GetDigestLength(Algorithm);
    CheckImpl(Output.size() >= requiredSize, "Output buffer too small for hash algorithm");
    SimdHashSingle(Algorithm, Input.size(), (const uint8_t*)Input.data(), Output.data());
}

}

#endif /* SimdHash_hpp */