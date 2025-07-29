#pragma once

#include <stdint.h>

namespace murmur {

// Returns i:th block (4 bytes) of the data array p as a 32-bit integer.
// Example:
//      if const char *p = "abcd" (in hex: 0x61,0x62,0x63,0x64)
//      then get_block(p, 0) returns 0x64636261 as an unsigned integer.
//      Character order is reversed in order to comply with original
//      implementation.
constexpr uint32_t get_block(const char *p, unsigned i) {
    const uint32_t block = 
        static_cast<uint32_t>(p[0+i*4]) << 0  | 
        static_cast<uint32_t>(p[1+i*4]) << 8  | 
        static_cast<uint32_t>(p[2+i*4]) << 16 | 
        static_cast<uint32_t>(p[3+i*4]) << 24;
    return block;
}

constexpr uint32_t rotl32(uint32_t x, int8_t r) {
    return (x << r) | (x >> (32 - r));
}

constexpr uint32_t fmix32(uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

constexpr uint32_t MurmurHash3_x86_32(const char *key,
                                      const unsigned len,
                                      const uint32_t seed) {
    const unsigned nblocks = len/4;

    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    //----------
    // body
    for (unsigned i = 0; i < nblocks; ++i) {
        uint32_t k1 = get_block(key, i);

        k1 *= c1;
        k1 = rotl32(k1,15);
        k1 *= c2;

        h1 ^= k1;
        h1 = rotl32(h1,13); 
        h1 = h1*5+0xe6546b64;
    }
    //----------
    // tail

    uint32_t k1 = 0;

    // The "tail" of key are the bytes that do not fit into any block,
    // len%4 is the size of the tail and 
    // 	(tail_start + i)
    // returns the i:th tail byte.
    const unsigned tail_start = len - (len % 4);
    switch (len & 3) {
    case 3: 
        k1 ^= key[tail_start + 2] << 16;
        [[fallthrough]];
    case 2: 
        k1 ^= key[tail_start + 1] << 8;
        [[fallthrough]];
    case 1: 
        k1 ^= key[tail_start + 0];
        k1 *= c1; k1 = rotl32(k1,15);
        k1 *= c2; h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= len;

    h1 = fmix32(h1);

    return h1;
}

// Compile time hash
template<uint64_t N>
constexpr uint32_t hash(const char(&s)[N]) {
    // TODO: Change seed?
    return murmur::MurmurHash3_x86_32(s, N-1, 123321u);
}

// Runtime hash
uint32_t hash(const std::string_view s) {
    // call the non-constexpr MurmurHash on a runtime string
    return murmur::MurmurHash3_x86_32(s.data(), s.size(), 123321u);
}

}