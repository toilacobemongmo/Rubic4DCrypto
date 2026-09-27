#include "../include/rubik4d_gcm.h"
#include "../include/rubik4d_simd.h"
#include <string.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

// Nhân trường Galois GF(2^128) theo chuẩn NIST SP 800-38D (Đa thức tối giản R = 0xE1 || 0^120)
static void gf128_mult_scalar(const uint8_t x[16], const uint8_t y[16], uint8_t res[16]) {
    uint8_t z[16] = {0};
    uint8_t v[16];
    memcpy(v, y, 16);

    for (int i = 0; i < 128; i++) {
        int bit = (x[i / 8] >> (7 - (i % 8))) & 1;
        if (bit) {
            for (int j = 0; j < 16; j++) {
                z[j] ^= v[j];
            }
        }

        int lsb = v[15] & 1;
        for (int j = 15; j > 0; j--) {
            v[j] = (v[j] >> 1) | ((v[j - 1] & 1) << 7);
        }
        v[0] >>= 1;
        if (lsb) {
            v[0] ^= 0xE1;
        }
    }
    memcpy(res, z, 16);
}

// Khởi tạo bảng tra cứu 16x256 để tăng tốc GHASH lên hàng GB/s
static void init_h_table(rubik4d_gcm_ctx *ctx) {
    for (int pos = 0; pos < 16; pos++) {
        for (int b = 0; b < 256; b++) {
            uint8_t x[16] = {0};
            x[pos] = (uint8_t)b;
            gf128_mult_scalar(x, ctx->H, ctx->H_table[pos][b]);
        }
    }
}

// Hàm băm đa thức GHASH tối ưu hóa bằng bảng 16x256 và vector XOR
static void ghash_update_fast(const rubik4d_gcm_ctx *ctx, const uint8_t *data, size_t len, uint8_t y[16]) {
    size_t num_blocks = len / 16;
    for (size_t b = 0; b < num_blocks; b++) {
        const uint8_t *blk = data + b * 16;
        __m128i z = _mm_setzero_si128();
        for (int i = 0; i < 16; i++) {
            uint8_t idx = y[i] ^ blk[i];
            __m128i entry = _mm_loadu_si128((const __m128i*)ctx->H_table[i][idx]);
            z = _mm_xor_si128(z, entry);
        }
        _mm_storeu_si128((__m128i*)y, z);
    }

    size_t rem = len % 16;
    if (rem > 0) {
        uint8_t last_block[16] = {0};
        memcpy(last_block, data + num_blocks * 16, rem);
        __m128i z = _mm_setzero_si128();
        for (int i = 0; i < 16; i++) {
            uint8_t idx = y[i] ^ last_block[i];
            __m128i entry = _mm_loadu_si128((const __m128i*)ctx->H_table[i][idx]);
            z = _mm_xor_si128(z, entry);
        }
        _mm_storeu_si128((__m128i*)y, z);
    }
}

static inline void inc32(uint8_t b[16]) {
    for (int i = 15; i >= 12; i--) {
        if (++b[i] != 0) break;
    }
}

// Khởi tạo context Rubik4D-GCM
void rubik4d_gcm_init(rubik4d_gcm_ctx *ctx, const uint8_t *key, size_t key_len) {
    rubik4d_key_setup(&ctx->cipher_ctx, key, key_len);

    // Tính Hash Subkey H = Rubik4D_K(0^128)
    uint8_t zero_block[16] = {0};
    rubik4d_encrypt_block_fast(&ctx->cipher_ctx, zero_block, ctx->H);

    // Xây dựng bảng tra cứu GHASH Shoup 16x256
    init_h_table(ctx);
    ctx->table_ready = 1;
}

// Tính J0 từ IV (Hỗ trợ IV 96-bit chuẩn và IV độ dài tùy ý)
static void compute_j0(const rubik4d_gcm_ctx *ctx, const uint8_t *iv, size_t iv_len, uint8_t j0[16]) {
    if (iv_len == 12) {
        memcpy(j0, iv, 12);
        j0[12] = 0;
        j0[13] = 0;
        j0[14] = 0;
        j0[15] = 1;
    } else {
        memset(j0, 0, 16);
        ghash_update_fast(ctx, iv, iv_len, j0);
        uint8_t len_block[16] = {0};
        uint64_t bit_len = (uint64_t)iv_len * 8;
        for (int i = 0; i < 8; i++) {
            len_block[15 - i] = (uint8_t)(bit_len >> (i * 8));
        }
        ghash_update_fast(ctx, len_block, 16, j0);
    }
}

// Mã hóa xác thực Rubik4D-GCM tốc độ cao
int rubik4d_gcm_encrypt(rubik4d_gcm_ctx *ctx,
                        const uint8_t *iv, size_t iv_len,
                        const uint8_t *aad, size_t aad_len,
                        const uint8_t *plain, size_t plain_len,
                        uint8_t *cipher, uint8_t tag[16]) {
    uint8_t j0[16];
    compute_j0(ctx, iv, iv_len, j0);

    // 1. Mã hóa CTR bằng vector engine AVX2: Bắt đầu từ J0 + 1
    uint8_t cb[16];
    memcpy(cb, j0, 16);
    inc32(cb);

    if (plain_len > 0) {
        rubik4d_ctr_encrypt_avx2(&ctx->cipher_ctx, plain, cipher, plain_len, cb);
    }

    // 2. Tính GHASH trên AAD và Ciphertext bằng bảng tra cứu 16x256 cực nhanh
    uint8_t s[16] = {0};
    if (aad && aad_len > 0) {
        ghash_update_fast(ctx, aad, aad_len, s);
    }
    if (cipher && plain_len > 0) {
        ghash_update_fast(ctx, cipher, plain_len, s);
    }

    // Ghép độ dài bit của AAD và Ciphertext (64-bit Big-Endian mỗi trường)
    uint8_t len_block[16] = {0};
    uint64_t aad_bits = (uint64_t)aad_len * 8;
    uint64_t cipher_bits = (uint64_t)plain_len * 8;
    for (int i = 0; i < 8; i++) {
        len_block[7 - i] = (uint8_t)(aad_bits >> (i * 8));
        len_block[15 - i] = (uint8_t)(cipher_bits >> (i * 8));
    }
    ghash_update_fast(ctx, len_block, 16, s);

    // 3. Tính Authentication Tag: Tag = S ^ Rubik4D_K(J0)
    uint8_t enc_j0[16];
    rubik4d_encrypt_block_fast(&ctx->cipher_ctx, j0, enc_j0);
    for (int i = 0; i < 16; i++) {
        tag[i] = s[i] ^ enc_j0[i];
    }

    return 0;
}

// So sánh 16-byte hằng số thời gian (Constant-time comparison) chống tấn công thời gian
static int constant_time_memcmp16(const uint8_t a[16], const uint8_t b[16]) {
    volatile uint8_t diff = 0;
    for (int i = 0; i < 16; i++) {
        diff |= (a[i] ^ b[i]);
    }
    return (diff == 0) ? 0 : -1;
}

// Giải mã và xác thực Rubik4D-GCM
int rubik4d_gcm_decrypt(rubik4d_gcm_ctx *ctx,
                        const uint8_t *iv, size_t iv_len,
                        const uint8_t *aad, size_t aad_len,
                        const uint8_t *cipher, size_t cipher_len,
                        const uint8_t tag[16], uint8_t *plain) {
    uint8_t j0[16];
    compute_j0(ctx, iv, iv_len, j0);

    // 1. Tính toán lại Tag mong đợi từ AAD và Ciphertext
    uint8_t s[16] = {0};
    if (aad && aad_len > 0) {
        ghash_update_fast(ctx, aad, aad_len, s);
    }
    if (cipher && cipher_len > 0) {
        ghash_update_fast(ctx, cipher, cipher_len, s);
    }

    uint8_t len_block[16] = {0};
    uint64_t aad_bits = (uint64_t)aad_len * 8;
    uint64_t cipher_bits = (uint64_t)cipher_len * 8;
    for (int i = 0; i < 8; i++) {
        len_block[7 - i] = (uint8_t)(aad_bits >> (i * 8));
        len_block[15 - i] = (uint8_t)(cipher_bits >> (i * 8));
    }
    ghash_update_fast(ctx, len_block, 16, s);

    uint8_t enc_j0[16];
    rubik4d_encrypt_block_fast(&ctx->cipher_ctx, j0, enc_j0);
    uint8_t expected_tag[16];
    for (int i = 0; i < 16; i++) {
        expected_tag[i] = s[i] ^ enc_j0[i];
    }

    // 2. Kiểm tra tính toàn vẹn của Tag (Authentication Verification) trong thời gian hằng số
    if (constant_time_memcmp16(tag, expected_tag) != 0) {
        if (plain && cipher_len > 0) {
            memset(plain, 0, cipher_len); // Xóa trắng bộ nhớ bản rõ để bảo mật
        }
        return -1;
    }

    // 3. Xác thực thành công: Tiến hành giải mã CTR
    uint8_t cb[16];
    memcpy(cb, j0, 16);
    inc32(cb);

    if (cipher_len > 0) {
        rubik4d_ctr_encrypt_avx2(&ctx->cipher_ctx, cipher, plain, cipher_len, cb);
    }

    return 0;
}

// Giao diện One-shot tiện lợi
int rubik4d_aead_encrypt(const uint8_t *key, size_t key_len,
                         const uint8_t *iv, size_t iv_len,
                         const uint8_t *aad, size_t aad_len,
                         const uint8_t *plain, size_t plain_len,
                         uint8_t *cipher, uint8_t tag[16]) {
    rubik4d_gcm_ctx ctx;
    rubik4d_gcm_init(&ctx, key, key_len);
    return rubik4d_gcm_encrypt(&ctx, iv, iv_len, aad, aad_len, plain, plain_len, cipher, tag);
}

int rubik4d_aead_decrypt(const uint8_t *key, size_t key_len,
                         const uint8_t *iv, size_t iv_len,
                         const uint8_t *aad, size_t aad_len,
                         const uint8_t *cipher, size_t cipher_len,
                         const uint8_t tag[16], uint8_t *plain) {
    rubik4d_gcm_ctx ctx;
    rubik4d_gcm_init(&ctx, key, key_len);
    return rubik4d_gcm_decrypt(&ctx, iv, iv_len, aad, aad_len, cipher, cipher_len, tag, plain);
}
