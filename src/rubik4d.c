<<<<<<< HEAD
#include "../include/rubik4d.h"
#include <string.h>
#include <math.h>

// SBOX
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

// RCON cho 6 vong
static const uint8_t RCON[7] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20
};

static uint8_t INV_SBOX[256];
static uint8_t PERM_TABLES[12][16];
static uint8_t INV_PERM_TABLES[12][16];
static int tables_initialized = 0;

static inline uint8_t to_idx(int x, int y, int z, int w) { return (uint8_t)((x << 3) | (y << 2) | (z << 1) | w); }
static inline void from_idx(uint8_t i, int *x, int *y, int *z, int *w) { *x = (i >> 3) & 1; *y = (i >> 2) & 1; *z = (i >> 1) & 1; *w = i & 1; }

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
    for (int i = 0; i < 256; i++) INV_SBOX[SBOX[i]] = (uint8_t)i;

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

void rubik4d_generate_round_keys(const uint8_t *key, size_t key_len, uint8_t round_keys[7][16]) {
    (void)key_len;
    memcpy(round_keys[0], key, 16);

    for (int r = 1; r <= 6; r++) {
        uint8_t temp[4];
        temp[0] = round_keys[r-1][13];
        temp[1] = round_keys[r-1][14];
        temp[2] = round_keys[r-1][15];
        temp[3] = round_keys[r-1][12];

=======
#include "rubik4d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NUM_ROUNDS 6

// 1. HỘP S-BOX AES CHUẨN
static const uint8_t SBOX[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

// 2. HỘP INV S-BOX
static const uint8_t INV_SBOX[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

static const uint8_t RCON[7] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20};

static uint8_t PERM_TABLES[12][16];
static uint8_t INV_PERM_TABLES[12][16];

static void rot2d(uint8_t *u, uint8_t *v, int cw) {
    if (cw) {
        if (*u == 0 && *v == 0) { *u = 0; *v = 1; }
        else if (*u == 0 && *v == 1) { *u = 1; *v = 1; }
        else if (*u == 1 && *v == 1) { *u = 1; *v = 0; }
        else { *u = 0; *v = 0; }
    } else {
        if (*u == 0 && *v == 0) { *u = 1; *v = 0; }
        else if (*u == 1 && *v == 0) { *u = 1; *v = 1; }
        else if (*u == 1 && *v == 1) { *u = 0; *v = 1; }
        else { *u = 0; *v = 0; }
    }
}

void rubik4d_init_tables(void) {
    int tbl_idx = 0;
    for (int p = 0; p < 6; p++) {
        for (int d = 0; d < 2; d++) {
            int cw = (d == 0);
            for (int i = 0; i < 16; i++) {
                uint8_t x = (i >> 3) & 1;
                uint8_t y = (i >> 2) & 1;
                uint8_t z = (i >> 1) & 1;
                uint8_t w = i & 1;

                if (p == 0) rot2d(&x, &y, cw);
                else if (p == 1) rot2d(&x, &z, cw);
                else if (p == 2) rot2d(&x, &w, cw);
                else if (p == 3) rot2d(&y, &z, cw);
                else if (p == 4) rot2d(&y, &w, cw);
                else if (p == 5) rot2d(&z, &w, cw);

                uint8_t dest = (x << 3) | (y << 2) | (z << 1) | w;
                PERM_TABLES[tbl_idx][dest] = (uint8_t)i;
                INV_PERM_TABLES[tbl_idx][i] = dest;
            }
            tbl_idx++;
        }
    }
}

void rubik4d_generate_round_keys(const uint8_t *key, size_t key_len, uint8_t round_keys[7][16]) {
    uint8_t k[16] = {0};
    if (key && key_len > 0) {
        size_t copy_len = key_len > 16 ? 16 : key_len;
        memcpy(k, key, copy_len);
    }
    memcpy(round_keys[0], k, 16);

    for (int r = 1; r <= NUM_ROUNDS; r++) {
        uint8_t temp[4] = {
            round_keys[r - 1][13],
            round_keys[r - 1][14],
            round_keys[r - 1][15],
            round_keys[r - 1][12]
        };
>>>>>>> master
        temp[0] = SBOX[temp[0]] ^ RCON[r];
        temp[1] = SBOX[temp[1]];
        temp[2] = SBOX[temp[2]];
        temp[3] = SBOX[temp[3]];

<<<<<<< HEAD
        for (int i = 0; i < 4; i++) round_keys[r][i] = round_keys[r-1][i] ^ temp[i];
        for (int i = 4; i < 16; i++) round_keys[r][i] = round_keys[r-1][i] ^ round_keys[r][i-4];
=======
        for (int i = 0; i < 4; i++) {
            round_keys[r][i] = round_keys[r - 1][i] ^ temp[i];
        }
        for (int i = 4; i < 16; i++) {
            round_keys[r][i] = round_keys[r - 1][i] ^ round_keys[r][i - 4];
        }
>>>>>>> master
    }
}

void rubik4d_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t round_keys[7][16]) {
    uint8_t state[16];
<<<<<<< HEAD
    for (int i = 0; i < 16; i++) state[i] = in[i] ^ round_keys[0][i];

    for (int r = 1; r <= 6; r++) {
        for (int i = 0; i < 16; i++) state[i] = SBOX[state[i]];

        uint8_t k_fold = 0;
        for (int i = 0; i < 16; i++) k_fold ^= round_keys[r][i];
        int tbl_idx = ((r + (k_fold & 7)) % 6) * 2 + ((k_fold >> 3) & 1);
        const uint8_t *lut = PERM_TABLES[tbl_idx];

        uint8_t rot[16];
        for (int i = 0; i < 16; i++) rot[i] = state[lut[i]];

        for (int i = 0; i < 16; i++) {
            int next = (i + 1) & 15;
            rot[next] ^= (uint8_t)(rot[i] + 0x5a);
        }

        for (int i = 0; i < 16; i++) state[i] = rot[i] ^ round_keys[r][i];
    }
=======
    uint8_t s_box_out[16];
    uint8_t rot[16];

    for (int i = 0; i < 16; i++) {
        state[i] = in[i] ^ round_keys[0][i];
    }

    for (int r = 1; r <= NUM_ROUNDS; r++) {
        for (int i = 0; i < 16; i++) {
            s_box_out[i] = SBOX[state[i]];
        }

        uint8_t k_fold = 0;
        for (int i = 0; i < 16; i++) {
            k_fold ^= round_keys[r][i];
        }
        int tbl_idx = ((r + (k_fold & 7)) % 6) * 2 + ((k_fold >> 3) & 1);
        const uint8_t *lut = PERM_TABLES[tbl_idx];

        for (int i = 0; i < 16; i++) {
            rot[i] = s_box_out[lut[i]];
        }

        for (int i = 0; i < 16; i++) {
            int next_idx = (i + 1) & 15;
            rot[next_idx] ^= ((rot[i] + 0x5A) & 0xFF);
        }

        for (int i = 0; i < 16; i++) {
            state[i] = rot[i] ^ round_keys[r][i];
        }
    }

>>>>>>> master
    memcpy(out, state, 16);
}

void rubik4d_decrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t round_keys[7][16]) {
    uint8_t state[16];
<<<<<<< HEAD
    memcpy(state, in, 16);

    for (int r = 6; r >= 1; r--) {
        for (int i = 0; i < 16; i++) state[i] ^= round_keys[r][i];

        // Sửa lỗi đảo ngược Ripple-ARX chính xác:
        state[0] ^= (uint8_t)(state[15] + 0x5a);
        for (int i = 14; i >= 0; i--) {
            state[i + 1] ^= (uint8_t)(state[i] + 0x5a);
        }

        uint8_t k_fold = 0;
        for (int i = 0; i < 16; i++) k_fold ^= round_keys[r][i];
        int tbl_idx = ((r + (k_fold & 7)) % 6) * 2 + ((k_fold >> 3) & 1);
        const uint8_t *inv_lut = INV_PERM_TABLES[tbl_idx];

        uint8_t rot[16];
        for (int i = 0; i < 16; i++) rot[i] = state[inv_lut[i]];
        for (int i = 0; i < 16; i++) state[i] = INV_SBOX[rot[i]];
    }

    for (int i = 0; i < 16; i++) out[i] = state[i] ^ round_keys[0][i];
}

size_t rubik4d_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len, const uint8_t *iv) {
    rubik4d_init_tables();
    uint8_t rkeys[7][16];
    rubik4d_generate_round_keys(key, key_len, rkeys);

    uint8_t pad = 16 - (in_len % 16);
    size_t total_len = in_len + pad;

    uint8_t block[16];
    uint8_t current_iv[16];
    memcpy(current_iv, iv, 16);

    for (size_t i = 0; i < total_len; i += 16) {
        for (int j = 0; j < 16; j++) {
            size_t idx = i + j;
            block[j] = (idx < in_len) ? in[idx] : pad;
            block[j] ^= current_iv[j]; 
        }
        
        rubik4d_encrypt_block(block, out + i, rkeys);
        memcpy(current_iv, out + i, 16); 
    }
    return total_len;
}

size_t rubik4d_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len, const uint8_t *iv) {
    if (in_len == 0 || (in_len % 16) != 0) return 0;
    rubik4d_init_tables();
    uint8_t rkeys[7][16];
    rubik4d_generate_round_keys(key, key_len, rkeys);

    uint8_t current_iv[16];
    memcpy(current_iv, iv, 16);

    for (size_t i = 0; i < in_len; i += 16) {
        rubik4d_decrypt_block(in + i, out + i, rkeys);
        
        for(int j = 0; j < 16; j++) {
            out[i + j] ^= current_iv[j];
        }
        memcpy(current_iv, in + i, 16); 
    }

    uint8_t pad = out[in_len - 1];
    if (pad < 1 || pad > 16) return 0;
    for (size_t i = in_len - pad; i < in_len; i++) {
        if (out[i] != pad) return 0;
    }
=======
    uint8_t rot[16];
    uint8_t s_box_out[16];

    memcpy(state, in, 16);

    for (int r = NUM_ROUNDS; r >= 1; r--) {
        for (int i = 0; i < 16; i++) {
            rot[i] = state[i] ^ round_keys[r][i];
        }

        for (int i = 15; i >= 0; i--) {
            int next_idx = (i + 1) & 15;
            rot[next_idx] ^= ((rot[i] + 0x5A) & 0xFF);
        }

        uint8_t k_fold = 0;
        for (int i = 0; i < 16; i++) {
            k_fold ^= round_keys[r][i];
        }
        int tbl_idx = ((r + (k_fold & 7)) % 6) * 2 + ((k_fold >> 3) & 1);
        const uint8_t *inv_lut = INV_PERM_TABLES[tbl_idx];

        for (int i = 0; i < 16; i++) {
            s_box_out[i] = rot[inv_lut[i]];
        }

        for (int i = 0; i < 16; i++) {
            state[i] = INV_SBOX[s_box_out[i]];
        }
    }

    for (int i = 0; i < 16; i++) {
        out[i] = state[i] ^ round_keys[0][i];
    }
}

// MÃ HÓA BUFFER VỚI CHẾ ĐỘ CBC VÀ PKCS#7 PADDING
size_t rubik4d_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len, const uint8_t *iv) {
    uint8_t round_keys[7][16];
    rubik4d_generate_round_keys(key, key_len, round_keys);

    uint8_t iv_block[16];
    if (iv) {
        memcpy(iv_block, iv, 16);
    } else {
        memset(iv_block, 0, 16);
    }

    size_t pad_len = 16 - (in_len % 16);
    size_t total_len = in_len + pad_len;
    size_t blocks = total_len / 16;

    uint8_t block[16];
    for (size_t b = 0; b < blocks; b++) {
        if (b == blocks - 1) {
            size_t remaining = in_len - b * 16;
            memcpy(block, in + b * 16, remaining);
            memset(block + remaining, (uint8_t)pad_len, pad_len);
        } else {
            memcpy(block, in + b * 16, 16);
        }

        // CBC Mode: XOR với IV hoặc ciphertext block trước
        for (int i = 0; i < 16; i++) {
            block[i] ^= iv_block[i];
        }

        rubik4d_encrypt_block(block, out + b * 16, round_keys);
        memcpy(iv_block, out + b * 16, 16);
    }

    return total_len;
}

// GIẢI MÃ BUFFER VỚI CHẾ ĐỘ CBC VÀ PKCS#7 UNPADDING
size_t rubik4d_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len, const uint8_t *iv) {
    if (in_len == 0 || in_len % 16 != 0) return 0;

    uint8_t round_keys[7][16];
    rubik4d_generate_round_keys(key, key_len, round_keys);

    uint8_t iv_block[16];
    if (iv) {
        memcpy(iv_block, iv, 16);
    } else {
        memset(iv_block, 0, 16);
    }

    size_t blocks = in_len / 16;
    uint8_t block[16];
    uint8_t next_iv[16];

    for (size_t b = 0; b < blocks; b++) {
        memcpy(next_iv, in + b * 16, 16);
        rubik4d_decrypt_block(in + b * 16, block, round_keys);

        for (int i = 0; i < 16; i++) {
            block[i] ^= iv_block[i];
        }
        memcpy(out + b * 16, block, 16);
        memcpy(iv_block, next_iv, 16);
    }

    // Gỡ PKCS#7 padding
    uint8_t pad = out[in_len - 1];
    if (pad > 16 || pad == 0) return in_len;
    for (size_t i = 0; i < pad; i++) {
        if (out[in_len - 1 - i] != pad) return in_len;
    }

>>>>>>> master
    return in_len - pad;
}

double rubik4d_calculate_entropy(const uint8_t *data, size_t len) {
    if (len == 0) return 0.0;
    size_t freq[256] = {0};
<<<<<<< HEAD
    for (size_t i = 0; i < len; i++) freq[data[i]]++;
=======
    for (size_t i = 0; i < len; i++) {
        freq[data[i]]++;
    }
>>>>>>> master

    double entropy = 0.0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            double p = (double)freq[i] / (double)len;
<<<<<<< HEAD
            entropy -= p * log2(p);
=======
            entropy -= p * (log(p) / log(2.0));
>>>>>>> master
        }
    }
    return entropy;
}