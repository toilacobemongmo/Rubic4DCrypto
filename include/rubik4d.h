#ifndef RUBIK4D_H
#define RUBIK4D_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RUBIK4D_BLOCK_SIZE 16
#define RUBIK4D_KEY_SIZE   16
#define RUBIK4D_ROUNDS     8

// Context lưu trữ khóa vòng và bảng hoán vị đã tiền tính toán
typedef struct {
    uint8_t round_keys[9][16];
    const uint8_t* perm_lut[9]; // Bảng hoán vị tĩnh theo từng vòng
} rubik4d_ctx;

void rubik4d_init_tables(void);
double rubik4d_calculate_entropy(const uint8_t *data, size_t len);

void rubik4d_key_setup(rubik4d_ctx *ctx, const uint8_t *key, size_t key_len);

void rubik4d_encrypt_block_fast(const rubik4d_ctx *ctx, const uint8_t in[16], uint8_t out[16]);
void rubik4d_decrypt_block_fast(const rubik4d_ctx *ctx, const uint8_t in[16], uint8_t out[16]);

// Giao diện CBC đồng bộ
size_t rubik4d_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, 
                       const uint8_t *key, size_t key_len, const uint8_t *iv);
size_t rubik4d_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, 
                       const uint8_t *key, size_t key_len, const uint8_t *iv);
                       // Hàm tương thích ngược cho module analysis.cpp
void rubik4d_generate_round_keys(const uint8_t *key, size_t key_len, uint8_t round_keys[9][16]);
void rubik4d_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t round_keys[9][16]);

#ifdef __cplusplus
}
#endif

#endif // RUBIK4D_H