#ifndef CHACHA20_H
#define CHACHA20_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHACHA20_BLOCK_SIZE 64   // Kích thước khối sinh keystream (64 bytes)
#define CHACHA20_KEY_SIZE   32   // Khóa chính 256 bit chuẩn RFC
#define CHACHA20_IV_SIZE    12   // Nonce 96 bit (12 bytes)

typedef struct {
    uint32_t state[16];
} chacha20_ctx;

void chacha20_init(chacha20_ctx *ctx, const uint8_t key[32], const uint8_t nonce[12], uint32_t counter);
void chacha20_xor(chacha20_ctx *ctx, const uint8_t *in, uint8_t *out, size_t len);

// Wrapper chuẩn dùng chung giao diện benchmark
size_t chacha20_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, 
                        const uint8_t *key, size_t key_len, const uint8_t *iv);
size_t chacha20_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, 
                        const uint8_t *key, size_t key_len, const uint8_t *iv);

#ifdef __cplusplus
}
#endif

#endif // CHACHA20_H