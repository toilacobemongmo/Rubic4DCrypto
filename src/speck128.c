#include "../include/speck128.h"
#include <string.h>

#define ROR64(x, r) (((x) >> (r)) | ((x) << (64 - (r))))
#define ROL64(x, r) (((x) << (r)) | ((x) >> (64 - (r))))
#define SPECK_ROUND(x, y, k) (x = (ROR64(x, 8) + y) ^ (k), y = ROL64(y, 3) ^ x)
#define SPECK_INV_ROUND(x, y, k) (y = ROR64(x ^ y, 3), x = ROL64((x ^ (k)) - y, 8))

void speck128_expand_key(const uint8_t key[16], uint64_t round_keys[32]) {
    uint64_t b, a;
    memcpy(&b, key, 8);
    memcpy(&a, key + 8, 8);
    round_keys[0] = b;
    for (uint64_t i = 0; i < 31; i++) {
        SPECK_ROUND(a, b, i);
        round_keys[i + 1] = b;
    }
}

void speck128_generate_round_keys(const uint8_t *key, size_t key_len, uint64_t round_keys[32]) {
    (void)key_len;
    speck128_expand_key(key, round_keys);
}

void speck128_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint64_t round_keys[32]) {
    uint64_t y, x;
    memcpy(&y, in, 8);
    memcpy(&x, in + 8, 8);
    for (int i = 0; i < 32; i++) {
        SPECK_ROUND(x, y, round_keys[i]);
    }
    memcpy(out, &y, 8);
    memcpy(out + 8, &x, 8);
}

void speck128_decrypt_block(const uint8_t in[16], uint8_t out[16], const uint64_t round_keys[32]) {
    uint64_t y, x;
    memcpy(&y, in, 8);
    memcpy(&x, in + 8, 8);
    for (int i = 31; i >= 0; i--) {
        SPECK_INV_ROUND(x, y, round_keys[i]);
    }
    memcpy(out, &y, 8);
    memcpy(out + 8, &x, 8);
}

size_t speck128_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len, const uint8_t *iv) {
    uint8_t fmt_key[16] = {0};
    for (size_t i = 0; i < key_len; i++) fmt_key[i % 16] ^= key[i];

    uint64_t rkeys[32];
    speck128_expand_key(fmt_key, rkeys);

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
        speck128_encrypt_block(block, out + i, rkeys);
        memcpy(current_iv, out + i, 16);
    }
    return total_len;
}

size_t speck128_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len, const uint8_t *iv) {
    if (in_len == 0 || (in_len % 16) != 0) return 0;
    uint8_t fmt_key[16] = {0};
    for (size_t i = 0; i < key_len; i++) fmt_key[i % 16] ^= key[i];

    uint64_t rkeys[32];
    speck128_expand_key(fmt_key, rkeys);

    uint8_t current_iv[16];
    memcpy(current_iv, iv, 16);

    for (size_t i = 0; i < in_len; i += 16) {
        speck128_decrypt_block(in + i, out + i, rkeys);
        for (int j = 0; j < 16; j++) {
            out[i + j] ^= current_iv[j];
        }
        memcpy(current_iv, in + i, 16);
    }

    uint8_t pad = out[in_len - 1];
    if (pad < 1 || pad > 16) return 0;
    for (size_t i = in_len - pad; i < in_len; i++) {
        if (out[i] != pad) return 0;
    }
    return in_len - pad;
}