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

#include "simdhash.h"

namespace simdhash
{

std::array<HashAlgorithm, SimdHashAlgorithmCount>
SimdHashAlgorithms{
    HashAlgorithmMD4,
    HashAlgorithmMD5,
    HashAlgorithmSHA1,
    HashAlgorithmSHA256,
    HashAlgorithmSHA384,
    HashAlgorithmSHA512
};

}

#endif /* SimdHash_hpp */