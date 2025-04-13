
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
    uint8_t hashes[MAX_LANES * MAX_HASH_SIZE];
    uint8_t compare[MAX_HASH_SIZE];
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
        for (size_t a = 0; a < SimdHashAlgorithmCount && !failed; a++)
        {
            HashAlgorithm algorithm = SimdHashAlgorithms[a];
            if (algorithm == HashAlgorithmNTLM)
            {
                continue;
            }
            
            fprintf(stderr, "Testing %s\n", HashAlgorithmToString(algorithm));

            for (size_t i = 0; i < lanes; i++)
            {
                size_t length = ((size_t)rand()) % MAXLEN;
                lengths[i] = length;
                for (size_t j = 0; j < length; j++)
                {
                    buffers[i][j] = (uint8_t)(rand() & 0xff);
                }
            }

            SimdHash(
                algorithm,
                lengths,
                bufferptrs,
                hashes
            );

            for (size_t i = 0; i < lanes; i++)
            {
                const uint8_t* hash = &hashes[i * GetHashWidth(algorithm)];
                SimdHashSingle(algorithm, lengths[i], bufferptrs[i], compare);
                if (memcmp(hash, compare, GetHashWidth(algorithm)) != 0)
                {
                    fprintf(stderr, "B[!] (length %3zu) ", lengths[i]);
                    print_hex(stderr, hash, 8);
                    fprintf(stderr, " != ");
                    print_hex(stderr, compare, 8);
                    fprintf(stderr, "\n");
                    failed = 1;
                }
                else
                {
                    fprintf(stderr, "B[+] (length %3zu) ", lengths[i]);
                    print_hex(stderr, hash, 8);
                    fprintf(stderr, " == ");
                    print_hex(stderr, compare, 8);
                    fprintf(stderr, "\n");
                }
            }

            if (SupportsOptimization(algorithm))
            {
                SimdHashOptimized(
                    algorithm,
                    lengths,
                    bufferptrs,
                    hashes
                );

                for (size_t i = 0; i < lanes; i++)
                {
                    const uint8_t* hash = &hashes[i * GetHashWidth(algorithm)];
                    SimdHashSingle(algorithm, lengths[i], bufferptrs[i], compare);
                    if (memcmp(hash, compare, GetHashWidth(algorithm)) != 0)
                    {
                        fprintf(stderr, "O[!] (length %3zu) ", lengths[i]);
                        print_hex(stderr, hash, 8);
                        fprintf(stderr, " != ");
                        print_hex(stderr, compare, 8);
                        fprintf(stderr, "\n");
                        failed = 1;
                    }
                    else
                    {
                        fprintf(stderr, "O[+] (length %3zu) ", lengths[i]);
                        print_hex(stderr, hash, 8);
                        fprintf(stderr, " == ");
                        print_hex(stderr, compare, 8);
                        fprintf(stderr, "\n");
                    }
                }
            }
        }
    }  
}