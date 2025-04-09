#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "simdhash.h"

#define EXTENDEDSIZE (256/8)

const char* string = "abc";
const size_t length = 3;

int main(int argc, char* argv[])
{
    uint8_t buffer[EXTENDEDSIZE * SimdLanes()];
    
    uint8_t* bufferpointers[MAX_LANES];
    size_t lengths[MAX_LANES];

    for (size_t i = 0; i < SimdLanes(); i++)
    {
        bufferpointers[i] = (uint8_t*)string;
        lengths[i] = length;
    }

    SimdHashExtended(
        HashAlgorithmSHA1,
        lengths,
        (const uint8_t* const *)bufferpointers,
        buffer,
        EXTENDEDSIZE / sizeof(uint32_t)
    );

    for (size_t i = 0; i < SimdLanes(); i++)
    {
        uint8_t single[EXTENDEDSIZE];
        SimdHashSingleExtended(
            HashAlgorithmSHA1,
            lengths[i],
            bufferpointers[i],
            single,
            EXTENDEDSIZE / sizeof(uint32_t)
        );
        if (memcmp(buffer + (i * EXTENDEDSIZE), single, EXTENDEDSIZE))
        {
            printf("Hash mismatch for lane %zu\n", i);
            // Print the hashes
            printf("Expected: ");
            for (size_t j = 0; j < EXTENDEDSIZE; j++)
            {
                printf("%02x", buffer[(i * EXTENDEDSIZE) + j]);
            }
            printf("\n");
            printf("Actual:   ");
            for (size_t j = 0; j < EXTENDEDSIZE; j++)
            {
                printf("%02x", single[j]);
            }
            printf("\n");
            return 1;
        }
        else
        {
            printf("Hash match for lane %zu\n", i);
        }
    }

    return 0;
}