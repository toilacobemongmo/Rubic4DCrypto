#ifndef SIMON128_H
#define SIMON128_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SIMON128_BLOCK_SIZE  16   // Khối 128 bit (hai nhánh 64 bit x, y)
#define SIMON128_KEY_SIZE    16   // Khóa chính 128 bit
#define SIMON128_ROUNDS      68   // 68 vòng chuẩn của SIMON-128/128
#define SIMON128_IV_SIZE     16   // Vector khởi tạo CBC 16 byte

void simon128_generate_round_keys(const uint8_t *key, size_t key_len, uint64_t round_keys[SIMON128_ROUNDS]);
void simon128_encrypt_block(const uint8_t in[SIMON128_BLOCK_SIZE], 
                            uint8_t out[SIMON128_BLOCK_SIZE], 
                            const uint64_t round_keys[SIMON128_ROUNDS]);
void simon128_decrypt_block(const uint8_t in[SIMON128_BLOCK_SIZE], 
                            uint8_t out[SIMON128_BLOCK_SIZE], 
                            const uint64_t round_keys[SIMON128_ROUNDS]);

size_t simon128_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, 
                        const uint8_t *key, size_t key_len, const uint8_t *iv);
size_t simon128_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, 
                        const uint8_t *key, size_t key_len, const uint8_t *iv);

#ifdef __cplusplus
}
#endif

#endif // SIMON128_H