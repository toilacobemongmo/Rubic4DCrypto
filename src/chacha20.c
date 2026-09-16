#include "../include/chacha20.h"
#include <string.h>

#define ROTL32(v, n) (((v) << (n)) | ((v) >> (32 - (n))))

#define QR(a, b, c, d) do { \
    a += b; d ^= a; d = ROTL32(d, 16); \
    c += d; b ^= c; b = ROTL32(b, 12); \
    a += b; d ^= a; d = ROTL32(d, 8);  \
    c += d; b ^= c; b = ROTL32(b, 7);  \
} while (0)

static inline uint32_t load32_le(const uint8_t *p) {
    return ((uint32_t)p[0])       |
           ((uint32_t)p[1] << 8)  |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static inline void store32_le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

void chacha20_init(chacha20_ctx *ctx, const uint8_t key[32], const uint8_t nonce[12], uint32_t counter) {
    // "expand 32-byte k" constants
    ctx->state[0] = 0x61707865;
    ctx->state[1] = 0x3320646e;
    ctx->state[2] = 0x79622d32;
    ctx->state[3] = 0x6b206574;

    for (int i = 0; i < 8; i++) {
        ctx->state[4 + i] = load32_le(key + i * 4);
    }

    ctx->state[12] = counter;
    ctx->state[13] = load32_le(nonce + 0);
    ctx->state[14] = load32_le(nonce + 4);
    ctx->state[15] = load32_le(nonce + 8);
}

static void chacha20_block(const chacha20_ctx *ctx, uint8_t output[64]) {
    uint32_t x[16];
    memcpy(x, ctx->state, sizeof(x));

    for (int i = 0; i < 10; i++) {
        // Column rounds
        QR(x[0], x[4], x[8],  x[12]);
        QR(x[1], x[5], x[9],  x[13]);
        QR(x[2], x[6], x[10], x[14]);
        QR(x[3], x[7], x[11], x[15]);
        // Diagonal rounds
        QR(x[0], x[5], x[10], x[15]);
        QR(x[1], x[6], x[11], x[12]);
        QR(x[2], x[7], x[8],  x[13]);
        QR(x[3], x[4], x[9],  x[14]);
    }

    for (int i = 0; i < 16; i++) {
        store32_le(output + i * 4, x[i] + ctx->state[i]);
    }
}

void chacha20_xor(chacha20_ctx *ctx, const uint8_t *in, uint8_t *out, size_t len) {
    uint8_t block[64];
    while (len > 0) {
        chacha20_block(ctx, block);
        ctx->state[12]++; // Tăng block counter

        size_t n = (len < 64) ? len : 64;
        for (size_t i = 0; i < n; i++) {
            out[i] = in[i] ^ block[i];
        }
        len -= n;
        in += n;
        out += n;
    }
}

static void derive_key_nonce(const uint8_t *key_in, size_t klen, uint8_t key_out[32],
                             const uint8_t *iv_in, uint8_t nonce_out[12]) {
    memset(key_out, 0, 32);
    for (size_t i = 0; i < klen; i++) key_out[i % 32] ^= key_in[i];

    if (iv_in) memcpy(nonce_out, iv_in, 12);
    else memset(nonce_out, 0, 12);
}

size_t chacha20_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, 
                        const uint8_t *key, size_t key_len, const uint8_t *iv) {
    uint8_t k[32], n[12];
    derive_key_nonce(key, key_len, k, iv, n);

    chacha20_ctx ctx;
    chacha20_init(&ctx, k, n, 1);
    chacha20_xor(&ctx, in, out, in_len);
    return in_len; // Stream cipher không cần padding
}

size_t chacha20_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, 
                        const uint8_t *key, size_t key_len, const uint8_t *iv) {
    // Mã dòng đối xứng: giải mã giống hệt mã hóa
    return chacha20_encrypt(in, in_len, out, key, key_len, iv);
}