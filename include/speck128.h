#ifndef SPECK128_H
#define SPECK128_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPECK128_BLOCK_SIZE  16
#define SPECK128_KEY_SIZE    16
#define SPECK128_ROUNDS      32

void speck128_expand_key(const uint8_t key[16], uint64_t round_keys[32]);
void speck128_generate_round_keys(const uint8_t *key, size_t key_len, uint64_t round_keys[32]);

void speck128_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint64_t round_keys[32]);
void speck128_decrypt_block(const uint8_t in[16], uint8_t out[16], const uint64_t round_keys[32]);

size_t speck128_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len, const uint8_t *iv);
size_t speck128_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, const uint8_t *key, size_t key_len, const uint8_t *iv);

#ifdef __cplusplus
}
#endif

#endif // SPECK128_H