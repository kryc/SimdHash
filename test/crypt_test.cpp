//
// crypt_test.cpp
// Port of simdhashtest_crypt.c to Google Test
// Tests SimdHash vs SimdHashSingle with random inputs
//

#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "simdhash.h"
}

#define MAXLEN 50
#define ITERATIONS 100

class CryptTest : public ::testing::TestWithParam<HashAlgorithm> {};

static std::string AlgoName(const ::testing::TestParamInfo<HashAlgorithm>& info) {
    return HashAlgorithmToString(info.param);
}

TEST_P(CryptTest, SimdHashMatchesSingle) {
    HashAlgorithm algo = GetParam();
    if (algo == HashAlgorithmNTLM) {
        GTEST_SKIP() << "NTLM skipped in crypt test (matches original)";
    }

    size_t lanes = SimdLanes();
    size_t digestLen = GetHashWidth(algo);

    srand(42); // Fixed seed for reproducibility

    for (int iter = 0; iter < ITERATIONS; iter++) {
        size_t lengths[MAX_LANES];
        uint8_t buffers[MAX_LANES][MAXLEN];
        const uint8_t* bufferptrs[MAX_LANES];
        uint8_t hashes[MAX_LANES * MAX_HASH_SIZE];
        uint8_t compare[MAX_HASH_SIZE];

        for (size_t i = 0; i < MAX_LANES; i++) {
            bufferptrs[i] = &buffers[i][0];
        }

        for (size_t i = 0; i < lanes; i++) {
            lengths[i] = ((size_t)rand()) % MAXLEN;
            for (size_t j = 0; j < lengths[i]; j++) {
                buffers[i][j] = (uint8_t)(rand() & 0xff);
            }
        }

        SimdHash(algo, lengths, bufferptrs, hashes);

        for (size_t i = 0; i < lanes; i++) {
            SimdHashSingle(algo, lengths[i], bufferptrs[i], compare);
            ASSERT_EQ(0, memcmp(&hashes[i * digestLen], compare, digestLen))
                << "Batch mismatch: algo=" << HashAlgorithmToString(algo)
                << " iter=" << iter << " lane=" << i
                << " length=" << lengths[i];
        }

        if (SupportsOptimization(algo)) {
            SimdHashOptimized(algo, lengths, bufferptrs, hashes);

            for (size_t i = 0; i < lanes; i++) {
                SimdHashSingle(algo, lengths[i], bufferptrs[i], compare);
                ASSERT_EQ(0, memcmp(&hashes[i * digestLen], compare, digestLen))
                    << "Optimized mismatch: algo=" << HashAlgorithmToString(algo)
                    << " iter=" << iter << " lane=" << i
                    << " length=" << lengths[i];
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    Crypt, CryptTest,
    ::testing::Values(
        HashAlgorithmMD4, HashAlgorithmMD5, HashAlgorithmSHA1,
        HashAlgorithmSHA256, HashAlgorithmSHA384, HashAlgorithmSHA512,
        HashAlgorithmNTLM
    ),
    AlgoName
);
