#ifndef AES128_H
#define AES128_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void aes128_key_expansion(const uint8_t key[16], uint8_t round_keys[176]);
void aes128_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t round_keys[176]);
void aes128_decrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t round_keys[176]);

size_t aes128_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len);
size_t aes128_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len);

#ifdef __cplusplus
}
#endif

#endif // AES128_H