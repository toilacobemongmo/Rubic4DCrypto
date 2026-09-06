#include "rubik4d.h"
#include <string.h>
#include <math.h>

static const uint8_t SBOX[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static uint8_t INV_SBOX[256];
static uint8_t PERM_TABLES[12][16];
static uint8_t INV_PERM_TABLES[12][16];
static int tables_initialized = 0;

static inline uint8_t to_idx(int x, int y, int z, int w) {
    return (uint8_t)((x << 3) | (y << 2) | (z << 1) | w);
}

static inline void from_idx(uint8_t i, int *x, int *y, int *z, int *w) {
    *x = (i >> 3) & 1;
    *y = (i >> 2) & 1;
    *z = (i >> 1) & 1;
    *w = i & 1;
}

static inline void rot2d(int *u, int *v, int cw) {
    int nu, nv;
    if (cw) {
        if (*u == 0 && *v == 0) { nu = 0; nv = 1; }
        else if (*u == 0 && *v == 1) { nu = 1; nv = 1; }
        else if (*u == 1 && *v == 1) { nu = 1; nv = 0; }
        else { nu = 0; nv = 0; }
    } else {
        if (*u == 0 && *v == 0) { nu = 1; nv = 0; }
        else if (*u == 1 && *v == 0) { nu = 1; nv = 1; }
        else if (*u == 1 && *v == 1) { nu = 0; nv = 1; }
        else { nu = 0; nv = 0; }
    }
    *u = nu; *v = nv;
}

void rubik4d_init_tables(void) {
    if (tables_initialized) return;

    for (int i = 0; i < 256; i++) {
        INV_SBOX[SBOX[i]] = (uint8_t)i;
    }

    int tbl = 0;
    for (int p = 0; p < 6; p++) {
        for (int d = 0; d < 2; d++) {
            int cw = (d == 0);
            for (uint8_t i = 0; i < 16; i++) {
                int x, y, z, w;
                from_idx(i, &x, &y, &z, &w);
                switch (p) {
                    case 0: rot2d(&x, &y, cw); break;
                    case 1: rot2d(&x, &z, cw); break;
                    case 2: rot2d(&x, &w, cw); break;
                    case 3: rot2d(&y, &z, cw); break;
                    case 4: rot2d(&y, &w, cw); break;
                    case 5: rot2d(&z, &w, cw); break;
                }
                uint8_t dest = to_idx(x, y, z, w);
                PERM_TABLES[tbl][dest] = i;
                INV_PERM_TABLES[tbl][i] = dest;
            }
            tbl++;
        }
    }
    tables_initialized = 1;
}

void rubik4d_generate_round_keys(const uint8_t *key, size_t key_len, uint8_t round_keys[13][16]) {
    uint32_t seed = 0;
    for (size_t i = 0; i < key_len; i++) {
        seed = (seed * 31) + key[i];
    }

    for (uint32_t r = 0; r <= 12; r++) {
        uint32_t cur = seed + (r * 0x9e3779b9u);
        for (int i = 0; i < 16; i++) {
            cur = (cur * 1664525u) + 1013904223u;
            round_keys[r][i] = (uint8_t)(cur >> 24);
        }
    }
}

void rubik4d_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t round_keys[13][16]) {
    uint8_t state[16];
    for (int i = 0; i < 16; i++) state[i] = in[i] ^ round_keys[0][i];

    for (int r = 1; r <= 12; r++) {
        for (int i = 0; i < 16; i++) state[i] = SBOX[state[i]];

        uint8_t k = round_keys[r][0];
        int tbl_idx = ((r + (k & 7)) % 6) * 2 + ((k >> 3) & 1);
        const uint8_t *lut = PERM_TABLES[tbl_idx];

        uint8_t rot[16];
        for (int i = 0; i < 16; i++) rot[i] = state[lut[i]];

        for (int i = 0; i < 16; i++) {
            int next = (i + 1) & 15;
            rot[next] ^= (uint8_t)(rot[i] + 0x5a);
        }

        for (int i = 0; i < 16; i++) state[i] = rot[i] ^ round_keys[r][i];
    }
    memcpy(out, state, 16);
}

void rubik4d_decrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t round_keys[13][16]) {
    uint8_t state[16];
    memcpy(state, in, 16);

    for (int r = 12; r >= 1; r--) {
        for (int i = 0; i < 16; i++) state[i] ^= round_keys[r][i];

        for (int i = 15; i >= 0; i--) {
            int next = (i + 1) & 15;
            state[next] ^= (uint8_t)(state[i] + 0x5a);
        }

        uint8_t k = round_keys[r][0];
        int tbl_idx = ((r + (k & 7)) % 6) * 2 + ((k >> 3) & 1);
        const uint8_t *inv_lut = INV_PERM_TABLES[tbl_idx];

        uint8_t rot[16];
        for (int i = 0; i < 16; i++) rot[i] = state[inv_lut[i]];

        for (int i = 0; i < 16; i++) state[i] = INV_SBOX[rot[i]];
    }

    for (int i = 0; i < 16; i++) out[i] = state[i] ^ round_keys[0][i];
}

size_t rubik4d_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len) {
    rubik4d_init_tables();
    uint8_t rkeys[13][16];
    rubik4d_generate_round_keys(key, key_len, rkeys);

    uint8_t pad = 16 - (in_len % 16);
    size_t total_len = in_len + pad;

    uint8_t block[16];
    for (size_t i = 0; i < total_len; i += 16) {
        for (int j = 0; j < 16; j++) {
            size_t idx = i + j;
            if (idx < in_len) block[j] = in[idx];
            else block[j] = pad;
        }
        rubik4d_encrypt_block(block, out + i, rkeys);
    }
    return total_len;
}

size_t rubik4d_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len) {
    if (in_len == 0 || (in_len % 16) != 0) return 0;
    rubik4d_init_tables();
    uint8_t rkeys[13][16];
    rubik4d_generate_round_keys(key, key_len, rkeys);

    for (size_t i = 0; i < in_len; i += 16) {
        rubik4d_decrypt_block(in + i, out + i, rkeys);
    }

    uint8_t pad = out[in_len - 1];
    if (pad < 1 || pad > 16) return 0;
    for (size_t i = in_len - pad; i < in_len; i++) {
        if (out[i] != pad) return 0;
    }
    return in_len - pad;
}

double rubik4d_calculate_entropy(const uint8_t *data, size_t len) {
    if (len == 0) return 0.0;
    size_t freq[256] = {0};
    for (size_t i = 0; i < len; i++) freq[data[i]]++;

    double entropy = 0.0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            double p = (double)freq[i] / (double)len;
            entropy -= p * log2(p);
        }
    }
    return entropy;
}