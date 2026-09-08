#ifndef RUBIK4D_H
#define RUBIK4D_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Khởi tạo các bảng hoán vị tĩnh
void rubik4d_init_tables(void);

// Sinh khóa vòng 128-bit chuẩn SPN
void rubik4d_generate_round_keys(const uint8_t *key, size_t key_len, uint8_t round_keys[7][16]);

// Mã hóa/Giải mã khối cơ sở 16-byte
void rubik4d_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t round_keys[7][16]);
void rubik4d_decrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t round_keys[7][16]);

// Hàm tiện ích mã hóa/giải mã buffer với PKCS#7 padding và chế độ hoạt động CBC
// ĐÃ FIX: Thêm tham số const uint8_t *iv (Initialization Vector 16-byte)
size_t rubik4d_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len, const uint8_t *iv);
size_t rubik4d_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len, const uint8_t *iv);

// Hàm tính Shannon Entropy
double rubik4d_calculate_entropy(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // RUBIK4D_H