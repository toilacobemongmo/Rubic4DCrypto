#include "../include/simon128.h"
#include <string.h>

#define ROL64(x, r) (((x) << (r)) | ((x) >> (64 - (r))))
#define ROR64(x, r) (((x) >> (r)) | ((x) << (64 - (r))))
#define SIMON_F(x)  ((ROL64(x, 1) & ROL64(x, 8)) ^ ROL64(x, 2))

static void derive_128bit_key(const uint8_t *key_in, size_t len, uint8_t key_out[16]) {
    memset(key_out, 0, 16);
    for (size_t i = 0; i < len; i++) {
        key_out[i % 16] ^= key_in[i];
    }
}

void simon128_generate_round_keys(const uint8_t *key, size_t key_len, uint64_t round_keys[SIMON128_ROUNDS]) {
    uint8_t formatted_key[16];
    derive_128bit_key(key, key_len, formatted_key);

    // Chuỗi hằng số z_4 (tuần hoàn chu kỳ 62)
    static const uint64_t z4 = 0x7369F885192C0EF5ULL;
    static const uint64_t c = 0xFFFFFFFFFFFFFFFCULL;

    uint64_t k[2];
    memcpy(&k[0], formatted_key, 8);
    memcpy(&k[1], formatted_key + 8, 8);

    round_keys[0] = k[0];
    round_keys[1] = k[1];

    for (int i = 2; i < SIMON128_ROUNDS; i++) {
        uint64_t tmp = ROR64(round_keys[i - 1], 3);
        tmp ^= round_keys[i - 2];
        tmp ^= ROR64(tmp, 1);
        uint64_t bit = (z4 >> ((i - 2) % 62)) & 1ULL;
        round_keys[i] = ~round_keys[i - 2] ^ tmp ^ c ^ bit;
    }
}

void simon128_encrypt_block(const uint8_t in[SIMON128_BLOCK_SIZE], 
                            uint8_t out[SIMON128_BLOCK_SIZE], 
                            const uint64_t round_keys[SIMON128_ROUNDS]) {
    uint64_t x, y;
    memcpy(&x, in + 8, 8);
    memcpy(&y, in, 8);

    for (int i = 0; i < SIMON128_ROUNDS; i++) {
        uint64_t tmp = y ^ SIMON_F(x) ^ round_keys[i];
        y = x;
        x = tmp;
    }

    memcpy(out, &y, 8);
    memcpy(out + 8, &x, 8);
}

void simon128_decrypt_block(const uint8_t in[SIMON128_BLOCK_SIZE], 
                            uint8_t out[SIMON128_BLOCK_SIZE], 
                            const uint64_t round_keys[SIMON128_ROUNDS]) {
    uint64_t x, y;
    memcpy(&y, in, 8);
    memcpy(&x, in + 8, 8);

    for (int i = SIMON128_ROUNDS - 1; i >= 0; i--) {
        uint64_t tmp = x ^ SIMON_F(y) ^ round_keys[i];
        x = y;
        y = tmp;
    }

    memcpy(out, &y, 8);
    memcpy(out + 8, &x, 8);
}

size_t simon128_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, 
                        const uint8_t *key, size_t key_len, const uint8_t *iv) {
    uint64_t round_keys[SIMON128_ROUNDS];
    simon128_generate_round_keys(key, key_len, round_keys);

    uint8_t pad = SIMON128_BLOCK_SIZE - (in_len % SIMON128_BLOCK_SIZE);
    size_t total_len = in_len + pad;

    uint8_t block[SIMON128_BLOCK_SIZE];
    uint8_t current_iv[SIMON128_BLOCK_SIZE];
    memcpy(current_iv, iv, SIMON128_BLOCK_SIZE);

    for (size_t i = 0; i < total_len; i += SIMON128_BLOCK_SIZE) {
        for (int j = 0; j < SIMON128_BLOCK_SIZE; j++) {
            size_t idx = i + j;
            block[j] = (idx < in_len) ? in[idx] : pad;
            block[j] ^= current_iv[j];
        }

        simon128_encrypt_block(block, out + i, round_keys);
        memcpy(current_iv, out + i, SIMON128_BLOCK_SIZE);
    }
    return total_len;
}

size_t simon128_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, 
                        const uint8_t *key, size_t key_len, const uint8_t *iv) {
    if (in_len == 0 || (in_len % SIMON128_BLOCK_SIZE) != 0) return 0;

    uint64_t round_keys[SIMON128_ROUNDS];
    simon128_generate_round_keys(key, key_len, round_keys);

    uint8_t current_iv[SIMON128_BLOCK_SIZE];
    memcpy(current_iv, iv, SIMON128_BLOCK_SIZE);

    for (size_t i = 0; i < in_len; i += SIMON128_BLOCK_SIZE) {
        simon128_decrypt_block(in + i, out + i, round_keys);

        for (int j = 0; j < SIMON128_BLOCK_SIZE; j++) {
            out[i + j] ^= current_iv[j];
        }
        memcpy(current_iv, in + i, SIMON128_BLOCK_SIZE);
    }

    uint8_t pad = out[in_len - 1];
    if (pad < 1 || pad > SIMON128_BLOCK_SIZE) return 0;
    for (size_t i = in_len - pad; i < in_len; i++) {
        if (out[i] != pad) return 0;
    }
    return in_len - pad;
}