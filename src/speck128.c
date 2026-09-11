#include "speck128.h"
#include <string.h>

#define ROR64(x, r) (((x) >> (r)) | ((x) << (64 - (r))))
#define ROL64(x, r) (((x) << (r)) | ((x) >> (64 - (r))))
#define R(x, y, k) (x = (ROR64(x, 8) + y) ^ k, y = ROL64(y, 3) ^ x)
#define RR(x, y, k) (y = ROR64(x ^ y, 3), x = ROL64((x ^ k) - y, 8))

static uint64_t load64_le(const uint8_t *b) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= ((uint64_t)b[i]) << (i * 8);
    return v;
}

static void store64_le(uint8_t *b, uint64_t v) {
    for (int i = 0; i < 8; i++) b[i] = (uint8_t)(v >> (i * 8));
}

void speck128_expand_key(const uint8_t key[16], uint64_t round_keys[32]) {
    uint64_t b = load64_le(key);
    uint64_t a = load64_le(key + 8);
    round_keys[0] = b;
    for (uint64_t i = 0; i < 31; i++) {
        R(a, b, i);
        round_keys[i + 1] = b;
    }
}

void speck128_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint64_t round_keys[32]) {
    uint64_t y = load64_le(in);
    uint64_t x = load64_le(in + 8);
    for (int i = 0; i < 32; i++) {
        R(x, y, round_keys[i]);
    }
    store64_le(out, y);
    store64_le(out + 8, x);
}

void speck128_decrypt_block(const uint8_t in[16], uint8_t out[16], const uint64_t round_keys[32]) {
    uint64_t y = load64_le(in);
    uint64_t x = load64_le(in + 8);
    for (int i = 31; i >= 0; i--) {
        RR(x, y, round_keys[i]);
    }
    store64_le(out, y);
    store64_le(out + 8, x);
}

size_t speck128_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len) {
    uint8_t k16[16] = {0};
    for (size_t i = 0; i < key_len; i++) k16[i % 16] ^= key[i];
    uint64_t rk[32];
    speck128_expand_key(k16, rk);

    uint8_t pad = 16 - (in_len % 16);
    size_t total_len = in_len + pad;
    uint8_t blk[16];
    for (size_t i = 0; i < total_len; i += 16) {
        for (int j = 0; j < 16; j++) {
            size_t idx = i + j;
            blk[j] = (idx < in_len) ? in[idx] : pad;
        }
        speck128_encrypt_block(blk, out + i, rk);
    }
    return total_len;
}

size_t speck128_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len) {
    if (in_len == 0 || (in_len % 16) != 0) return 0;
    uint8_t k16[16] = {0};
    for (size_t i = 0; i < key_len; i++) k16[i % 16] ^= key[i];
    uint64_t rk[32];
    speck128_expand_key(k16, rk);

    for (size_t i = 0; i < in_len; i += 16) {
        speck128_decrypt_block(in + i, out + i, rk);
    }
    uint8_t pad = out[in_len - 1];
    if (pad < 1 || pad > 16) return 0;
    return in_len - pad;
}