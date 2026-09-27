#ifndef RUBIK4D_SIMD_H
#define RUBIK4D_SIMD_H

#include "rubik4d.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Kiểm tra hỗ trợ phần cứng CPU tại thời điểm chạy (runtime CPUID)
int rubik4d_has_ssse3(void);
int rubik4d_has_avx2(void);
int rubik4d_has_pclmul(void);

// Mã hóa/giải mã đơn khối 128-bit tăng tốc bằng tập lệnh SSSE3 (_mm_shuffle_epi8)
void rubik4d_encrypt_block_simd(const rubik4d_ctx *ctx, const uint8_t in[16], uint8_t out[16]);
void rubik4d_decrypt_block_simd(const rubik4d_ctx *ctx, const uint8_t in[16], uint8_t out[16]);

// Mã hóa đa khối song song chế độ Counter (CTR) bằng AVX2 (4 khối 64-byte / vòng lặp)
void rubik4d_ctr_encrypt_avx2(const rubik4d_ctx *ctx, const uint8_t *in, uint8_t *out, size_t len, const uint8_t iv[16]);

#ifdef __cplusplus
}
#endif

#endif // RUBIK4D_SIMD_H
