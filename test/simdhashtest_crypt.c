
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "simdhash.h"

#define MAXLEN 50

void
print_hex(
    FILE* Out,
    const uint8_t* Buffer,
    const size_t Length
)
{
    for (size_t i = 0; i < Length; i++)
    {
        fprintf(Out, "%02x", Buffer[i]);
    }
}

int
main(
    int argc,
    char* argv[]
)
{
    size_t lengths[MAX_LANES];
    uint8_t buffers[MAX_LANES][MAXLEN];
    const uint8_t* bufferptrs[MAX_LANES];
    uint8_t hashes[MAX_LANES * SHA1_SIZE];
    uint8_t compare[SHA1_SIZE];
    size_t lanes = SimdLanes();
    int failed = 0;

    srand(
        argc == 2 ? atoi(argv[1]) : 3
    );

    for (size_t i = 0; i < MAX_LANES; i++)
    {
        bufferptrs[i] = &buffers[i][0];
    }

    while (!failed)
    {
        for (size_t i = 0; i < lanes; i++)
        {
            size_t length = ((size_t)rand()) % MAXLEN;
            lengths[i] = length;
            for (size_t j = 0; j < length; j++)
            {
                buffers[i][j] = (uint8_t)(rand() & 0xff);
            }
        }

        SimdHashOptimized(
            HashAlgorithmSHA1,
            lengths,
            bufferptrs,
            hashes
        );

        for (size_t i = 0; i < lanes; i++)
        {
            const uint8_t* hash = &hashes[i * SHA1_SIZE];
            SHA1(bufferptrs[i], lengths[i], compare);
            if (memcmp(hash, compare, SHA1_SIZE) != 0)
            {
                fprintf(stderr, "[!] (length %3zu) ", lengths[i]);
                print_hex(stderr, hash, 8);
                fprintf(stderr, " != ");
                print_hex(stderr, compare, 8);
                fprintf(stderr, "\n");
                failed = 1;
            }
            else
            {
                fprintf(stderr, "[+] (length %3zu) ", lengths[i]);
                print_hex(stderr, hash, 8);
                fprintf(stderr, " == ");
                print_hex(stderr, compare, 8);
                fprintf(stderr, "\n");
            }
        }
    }  
}