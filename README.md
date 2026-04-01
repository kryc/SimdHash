# SimdHash

A high-performance SIMD-accelerated hashing library that computes multiple hashes in parallel using CPU vector instructions.

SimdHash processes multiple input buffers simultaneously by packing them into SIMD lanes, achieving significant throughput improvements over scalar implementations.

## Supported Algorithms

| Algorithm | SIMD Parallel | Scalar Fallback |
|-----------|:---:|:---:|
| MD4       | ✓ | |
| MD5       | ✓ | |
| SHA-1     | ✓ | |
| SHA-256   | ✓ | |
| SHA-384   | | ✓ (OpenSSL) |
| SHA-512   | | ✓ (OpenSSL) |
| NTLM      | ✓ | |
| FNV-1 32  | ✓ | |
| FNV-1a 32 | ✓ | |
| FNV-1 64  | ✓ | |
| FNV-1a 64 | ✓ | |

## Supported SIMD Instruction Sets

- **AVX-512** — 16 lanes (512-bit)
- **AVX2** — 8 lanes (256-bit)
- **SSE4.2** — 4 lanes (128-bit)
- **SSSE3** — 4 lanes (128-bit)
- **SSE2** — 4 lanes (128-bit)
- **ARM NEON** — 4 lanes (128-bit)

By default, the build auto-detects the best instruction set for the host CPU via `-march=native`.

## Dependencies

- **Clang** (C/C++ compiler)
- **CMake** ≥ 3.14
- **OpenSSL** (`libcrypto`) — for SHA-384/SHA-512 and test verification
- **ICU** (`libicuuc`) — for Unicode/NTLM support

## Building

```bash
mkdir -p build && cd build
cmake ..
make
```

### Specifying a SIMD level

Use the `SIMD` variable to target a specific instruction set instead of auto-detecting:

```bash
cmake -DSIMD=avx512 ..   # AVX-512
cmake -DSIMD=avx256 ..   # AVX2
cmake -DSIMD=sse42 ..    # SSE4.2
cmake -DSIMD=ssse3 ..    # SSSE3
cmake -DSIMD=sse2 ..     # SSE2
```

### Build types

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..   # (default) optimized
cmake -DCMAKE_BUILD_TYPE=Debug ..     # debug + AddressSanitizer
```

## Usage

### C API

```c
#include "simdhash.h"

// Hash a single buffer
uint8_t digest[MD5_SIZE];
SimdHashSingle(HashAlgorithmMD5, length, buffer, digest);

// Hash multiple buffers in parallel (one per SIMD lane)
SimdHashContext ctx;
SimdHashInit(&ctx, HashAlgorithmSHA256);
SimdHashUpdate(&ctx, lengths, buffers);
SimdHashFinalize(&ctx);
// Digests are interleaved in ctx.H[]
```

### C++ API

```cpp
#include "SimdHash.hpp"
#include "SimdHashBuffer.hpp"

// Single hash
std::array<uint8_t, SHA256_SIZE> digest;
simdhash::SimdHashSingle(HashAlgorithmSHA256, input, digest);

// Batched hashing with SimdHashBuffer
SimdHashBuffer buf(MAX_BUFFER_SIZE);
buf.Set(0, "hello");
buf.Set(1, "world");
// ...
SimdHashContext ctx;
SimdHashInit(&ctx, HashAlgorithmMD5);
SimdHashUpdate(&ctx, buf.GetLengths(), buf.ConstBuffers());
SimdHashFinalize(&ctx);
```

## Testing

Tests use [Google Test](https://github.com/google/googletest) (fetched automatically by CMake).

```bash
cd build
make                # build library + tests
ctest               # run all tests
```

## License

Copyright © 2020–2025 Gareth Evans. All rights reserved.
