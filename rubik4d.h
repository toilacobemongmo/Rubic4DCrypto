#ifndef RUBIK4D_H
#define RUBIK4D_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void rubik4d_init_tables(void);
void rubik4d_generate_round_keys(const uint8_t *key, size_t key_len, uint8_t round_keys[13][16]);
void rubik4d_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t round_keys[13][16]);
void rubik4d_decrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t round_keys[13][16]);

// Hàm tiện ích mã hóa/giải mã buffer với PKCS#7 padding
size_t rubik4d_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len);
size_t rubik4d_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len);
double rubik4d_calculate_entropy(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // RUBIK4D_H