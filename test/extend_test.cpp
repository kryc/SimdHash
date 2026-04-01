//
// extend_test.cpp
// Port of simdhashtest_extend.c to Google Test
//

#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>

extern "C" {
#include "simdhash.h"
}

#define EXTENDED_SIZE (256 / 8)

TEST(Extended, SHA1ExtendedMatchesSingle) {
    const char* input = "abc";
    const size_t length = 3;
    const size_t lanes = SimdLanes();
    const size_t countDwords = EXTENDED_SIZE / sizeof(uint32_t);

    uint8_t buffer[EXTENDED_SIZE * MAX_LANES];
    const uint8_t* bufferptrs[MAX_LANES];
    size_t lengths[MAX_LANES];

    for (size_t i = 0; i < lanes; i++) {
        bufferptrs[i] = (const uint8_t*)input;
        lengths[i] = length;
    }

    SimdHashExtended(
        HashAlgorithmSHA1,
        lengths,
        bufferptrs,
        buffer,
        countDwords
    );

    for (size_t i = 0; i < lanes; i++) {
        uint8_t single[EXTENDED_SIZE];
        SimdHashSingleExtended(
            HashAlgorithmSHA1,
            lengths[i],
            bufferptrs[i],
            single,
            countDwords
        );
        EXPECT_EQ(0, memcmp(buffer + (i * EXTENDED_SIZE), single, EXTENDED_SIZE))
            << "Hash mismatch for lane " << i;
    }
}
