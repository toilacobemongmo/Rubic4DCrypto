#ifndef RUBIK4D_GCM_H
#define RUBIK4D_GCM_H

#include "rubik4d.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RUBIK4D_GCM_TAG_SIZE 16
#define RUBIK4D_GCM_IV_DEFAULT_SIZE 12 // 96-bit chuẩn NIST SP 800-38D

// Context lưu trữ khóa và Hash Subkey H cho Rubik4D-GCM
typedef struct {
    rubik4d_ctx cipher_ctx;
    uint8_t H[16]; // Hash subkey H = Rubik4D_K(0^128)
    uint8_t H_table[16][256][16]; // Bảng tra cứu Shoup 8-bit tăng tốc GHASH
    int table_ready;
} rubik4d_gcm_ctx;

// Khởi tạo ngữ cảnh GCM
void rubik4d_gcm_init(rubik4d_gcm_ctx *ctx, const uint8_t *key, size_t key_len);

// Mã hóa xác thực (Authenticated Encryption):
// Nhận vào: Plaintext, Associated Data (AAD), IV
// Trả về: Ciphertext và Authentication Tag 128-bit
int rubik4d_gcm_encrypt(rubik4d_gcm_ctx *ctx,
                        const uint8_t *iv, size_t iv_len,
                        const uint8_t *aad, size_t aad_len,
                        const uint8_t *plain, size_t plain_len,
                        uint8_t *cipher, uint8_t tag[16]);

// Giải mã xác thực (Authenticated Decryption & Verification):
// Kiểm tra tính toàn vẹn của Ciphertext, AAD và Tag trong thời gian hằng số.
// Trả về 0 nếu xác thực THÀNH CÔNG, trả về -1 nếu DỮ LIỆU BỊ GIẢ MẠO / SAI TAG.
int rubik4d_gcm_decrypt(rubik4d_gcm_ctx *ctx,
                        const uint8_t *iv, size_t iv_len,
                        const uint8_t *aad, size_t aad_len,
                        const uint8_t *cipher, size_t cipher_len,
                        const uint8_t tag[16], uint8_t *plain);

// Giao diện một bước tiện lợi (One-shot API)
int rubik4d_aead_encrypt(const uint8_t *key, size_t key_len,
                         const uint8_t *iv, size_t iv_len,
                         const uint8_t *aad, size_t aad_len,
                         const uint8_t *plain, size_t plain_len,
                         uint8_t *cipher, uint8_t tag[16]);

int rubik4d_aead_decrypt(const uint8_t *key, size_t key_len,
                         const uint8_t *iv, size_t iv_len,
                         const uint8_t *aad, size_t aad_len,
                         const uint8_t *cipher, size_t cipher_len,
                         const uint8_t tag[16], uint8_t *plain);

#ifdef __cplusplus
}
#endif

#endif // RUBIK4D_GCM_H
