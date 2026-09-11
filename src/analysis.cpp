#include "analysis.h"
#include <vector>
#include <random>
#include <cstring>
#include <cmath>

#include "rubik4d.h"
#include "aes128.h"
#include "speck128.h"

static inline int CountBitDifferences(const uint8_t* a, const uint8_t* b, size_t len) {
    int diff = 0;
    for (size_t i = 0; i < len; i++) {
        diff += __builtin_popcount(a[i] ^ b[i]);
    }
    return diff;
}

static void EncryptSingleBlock(AlgorithmType algo, const uint8_t plain[16], const uint8_t key[16], uint8_t cipher[16]) {
    if (algo == ALGO_RUBIK4D) {
        uint8_t rkeys[13][16];
        rubik4d_init_tables();
        rubik4d_generate_round_keys(key, 16, rkeys);
        rubik4d_encrypt_block(plain, cipher, rkeys);
    } else if (algo == ALGO_AES128) {
        uint8_t rkeys[176];
        aes128_key_expansion(key, rkeys);
        aes128_encrypt_block(plain, cipher, rkeys);
    } else if (algo == ALGO_SPECK128) {
        uint64_t rkeys[32];
        speck128_expand_key(key, rkeys);
        speck128_encrypt_block(plain, cipher, rkeys);
    }
}

SecurityMetrics AnalyzeCipherSecurity(AlgorithmType algo, int samples) {
    SecurityMetrics m = {0.0, 0.0, 0.0};
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> byte_dist(0, 255);
    std::uniform_int_distribution<int> bit_pos_dist(0, 127);

    uint8_t fixed_key[16];
    for (int i = 0; i < 16; i++) fixed_key[i] = (uint8_t)byte_dist(rng);

    // 1. Phép đo SAC (Lật 1 bit Plaintext)
    double total_sac = 0.0;
    for (int s = 0; s < samples; s++) {
        uint8_t p1[16], p2[16];
        for (int i = 0; i < 16; i++) p1[i] = (uint8_t)byte_dist(rng);
        memcpy(p2, p1, 16);
        int bit = bit_pos_dist(rng);
        p2[bit / 8] ^= (1 << (bit % 8));

        uint8_t c1[16], c2[16];
        EncryptSingleBlock(algo, p1, fixed_key, c1);
        EncryptSingleBlock(algo, p2, fixed_key, c2);

        total_sac += ((double)CountBitDifferences(c1, c2, 16) / 128.0) * 100.0;
    }
    m.sac = total_sac / samples;

    // 2. Phép đo NPCR & UACI (Lật 1 bit Key)
    double total_npcr = 0.0, total_uaci = 0.0;
    for (int s = 0; s < samples; s++) {
        uint8_t p[16];
        for (int i = 0; i < 16; i++) p[i] = (uint8_t)byte_dist(rng);

        uint8_t k1[16], k2[16];
        for (int i = 0; i < 16; i++) k1[i] = (uint8_t)byte_dist(rng);
        memcpy(k2, k1, 16);
        int bit = bit_pos_dist(rng);
        k2[bit / 8] ^= (1 << (bit % 8));

        uint8_t c1[16], c2[16];
        EncryptSingleBlock(algo, p, k1, c1);
        EncryptSingleBlock(algo, p, k2, c2);

        int diff_bytes = 0, abs_diff = 0;
        for (int i = 0; i < 16; i++) {
            if (c1[i] != c2[i]) diff_bytes++;
            abs_diff += abs((int)c1[i] - (int)c2[i]);
        }
        total_npcr += ((double)diff_bytes / 16.0) * 100.0;
        total_uaci += ((double)abs_diff / (16.0 * 255.0)) * 100.0;
    }
    m.npcr = total_npcr / samples;
    m.uaci = total_uaci / samples;

    return m;
}