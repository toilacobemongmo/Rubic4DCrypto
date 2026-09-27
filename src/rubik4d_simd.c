#include "../include/rubik4d_simd.h"
#include <string.h>
#include <immintrin.h>
#include <cpuid.h>

#define ROTL32(v, n) (((v) << (n)) | ((v) >> (32 - (n))))

// AES S-box chuẩn
static const uint8_t AES_SBOX[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static const uint8_t AES_INV_SBOX[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

// Kiểm tra CPU capabilities
int rubik4d_has_ssse3(void) {
    unsigned int eax, ebx, ecx, edx;
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) return 0;
    return (ecx & (1 << 9)) != 0; // SSSE3 (bit 9 of ECX)
}

int rubik4d_has_avx2(void) {
    unsigned int eax, ebx, ecx, edx;
    if (!__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) return 0;
    return (ebx & (1 << 5)) != 0; // AVX2 (bit 5 of EBX)
}

int rubik4d_has_pclmul(void) {
    unsigned int eax, ebx, ecx, edx;
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) return 0;
    return (ecx & (1 << 1)) != 0; // PCLMULQDQ (bit 1 of ECX)
}

// Bảng hoán vị nghịch đảo toàn cục phục vụ giải mã nhanh SIMD
static uint8_t SIMD_INV_PERM_TABLES[12][16];
static int simd_tables_init = 0;

static void init_simd_tables(void) {
    if (simd_tables_init) return;
    extern const uint8_t PERM_TABLES[12][16];
    for (int t = 0; t < 12; t++) {
        for (int i = 0; i < 16; i++) {
            SIMD_INV_PERM_TABLES[t][PERM_TABLES[t][i]] = (uint8_t)i;
        }
    }
    simd_tables_init = 1;
}

// Mã hóa 1 khối 128-bit sử dụng lệnh SSSE3 PSHUFB cho hoán vị SO(4)
void rubik4d_encrypt_block_simd(const rubik4d_ctx *ctx, const uint8_t in[16], uint8_t out[16]) {
    // 0. Initial Key Whitening
    __m128i v_state = _mm_xor_si128(_mm_loadu_si128((const __m128i*)in), 
                                     _mm_loadu_si128((const __m128i*)ctx->round_keys[0]));

    for (int r = 1; r <= 8; r++) {
        // 1. SubBytes: Thay thế phi tuyến 16 byte
        uint8_t temp[16];
        _mm_storeu_si128((__m128i*)temp, v_state);
        for (int i = 0; i < 16; i++) {
            temp[i] = AES_SBOX[temp[i]];
        }
        v_state = _mm_loadu_si128((const __m128i*)temp);

        // 2. SO(4) Permutation: Thực thi bằng 1 LỆNH DUY NHẤT _mm_shuffle_epi8 (1 chu kỳ clock)
        __m128i v_lut = _mm_loadu_si128((const __m128i*)ctx->perm_lut[r]);
        v_state = _mm_shuffle_epi8(v_state, v_lut);

        // 3. Tầng ARX Ripple 32-bit lan truyền số nhớ
        _mm_storeu_si128((__m128i*)temp, v_state);
        uint32_t* w = (uint32_t*)temp;
        w[1] ^= ROTL32(w[0] + 0x5A5A5A5AU, 7);
        w[2] ^= ROTL32(w[1] + 0x5A5A5A5AU, 11);
        w[3] ^= ROTL32(w[2] + 0x5A5A5A5AU, 13);
        w[0] ^= ROTL32(w[3] + 0x5A5A5A5AU, 17);
        v_state = _mm_loadu_si128((const __m128i*)temp);

        // 4. AddRoundKey: Cộng khóa con vòng
        __m128i v_rk = _mm_loadu_si128((const __m128i*)ctx->round_keys[r]);
        v_state = _mm_xor_si128(v_state, v_rk);
    }

    _mm_storeu_si128((__m128i*)out, v_state);
}

// Giải mã 1 khối 128-bit sử dụng lệnh SSSE3 PSHUFB cho hoán vị SO(4) nghịch đảo
void rubik4d_decrypt_block_simd(const rubik4d_ctx *ctx, const uint8_t in[16], uint8_t out[16]) {
    init_simd_tables();
    __m128i v_state = _mm_loadu_si128((const __m128i*)in);

    for (int r = 8; r >= 1; r--) {
        // 1. Tách khóa con vòng AddRoundKey
        __m128i v_rk = _mm_loadu_si128((const __m128i*)ctx->round_keys[r]);
        v_state = _mm_xor_si128(v_state, v_rk);

        // 2. Đảo ngược tầng ARX Ripple 32-bit
        uint8_t temp[16];
        _mm_storeu_si128((__m128i*)temp, v_state);
        uint32_t* w = (uint32_t*)temp;
        w[0] ^= ROTL32(w[3] + 0x5A5A5A5AU, 17);
        w[3] ^= ROTL32(w[2] + 0x5A5A5A5AU, 13);
        w[2] ^= ROTL32(w[1] + 0x5A5A5A5AU, 11);
        w[1] ^= ROTL32(w[0] + 0x5A5A5A5AU, 7);
        v_state = _mm_loadu_si128((const __m128i*)temp);

        // 3. Đảo ngược hoán vị SO(4)
        // Tìm bảng nghịch đảo tương ứng
        const uint8_t* lut = ctx->perm_lut[r];
        uint8_t inv_lut[16];
        for (int i = 0; i < 16; i++) {
            inv_lut[lut[i]] = (uint8_t)i;
        }
        __m128i v_inv_lut = _mm_loadu_si128((const __m128i*)inv_lut);
        v_state = _mm_shuffle_epi8(v_state, v_inv_lut);

        // 4. Nghịch đảo phi tuyến InvSubBytes
        _mm_storeu_si128((__m128i*)temp, v_state);
        for (int i = 0; i < 16; i++) {
            temp[i] = AES_INV_SBOX[temp[i]];
        }
        v_state = _mm_loadu_si128((const __m128i*)temp);
    }

    // Khôi phục bản rõ bằng Initial Key Whitening
    __m128i v_k0 = _mm_loadu_si128((const __m128i*)ctx->round_keys[0]);
    v_state = _mm_xor_si128(v_state, v_k0);
    _mm_storeu_si128((__m128i*)out, v_state);
}

// Tăng counter 32-bit ở cuối khối 16-byte
static inline void inc_counter_32(uint8_t ctr[16]) {
    for (int i = 15; i >= 12; i--) {
        if (++ctr[i] != 0) break;
    }
}

#define SUBBYTES_BLOCK(arr) do { \
    (arr)[0]  = AES_SBOX[(arr)[0]];  (arr)[1]  = AES_SBOX[(arr)[1]];  \
    (arr)[2]  = AES_SBOX[(arr)[2]];  (arr)[3]  = AES_SBOX[(arr)[3]];  \
    (arr)[4]  = AES_SBOX[(arr)[4]];  (arr)[5]  = AES_SBOX[(arr)[5]];  \
    (arr)[6]  = AES_SBOX[(arr)[6]];  (arr)[7]  = AES_SBOX[(arr)[7]];  \
    (arr)[8]  = AES_SBOX[(arr)[8]];  (arr)[9]  = AES_SBOX[(arr)[9]];  \
    (arr)[10] = AES_SBOX[(arr)[10]]; (arr)[11] = AES_SBOX[(arr)[11]]; \
    (arr)[12] = AES_SBOX[(arr)[12]]; (arr)[13] = AES_SBOX[(arr)[13]]; \
    (arr)[14] = AES_SBOX[(arr)[14]]; (arr)[15] = AES_SBOX[(arr)[15]]; \
} while(0)

// Mã hóa đa khối song song 4 khối (64 bytes/vòng lặp) tận dụng vector transpose ARX và pshufb
static void rubik4d_encrypt_4blocks_simd(const rubik4d_ctx *ctx, 
                                        const uint8_t in0[16], const uint8_t in1[16],
                                        const uint8_t in2[16], const uint8_t in3[16],
                                        uint8_t out0[16], uint8_t out1[16],
                                        uint8_t out2[16], uint8_t out3[16]) {
    __m128i rk0 = _mm_loadu_si128((const __m128i*)ctx->round_keys[0]);
    __m128i b0 = _mm_xor_si128(_mm_loadu_si128((const __m128i*)in0), rk0);
    __m128i b1 = _mm_xor_si128(_mm_loadu_si128((const __m128i*)in1), rk0);
    __m128i b2 = _mm_xor_si128(_mm_loadu_si128((const __m128i*)in2), rk0);
    __m128i b3 = _mm_xor_si128(_mm_loadu_si128((const __m128i*)in3), rk0);

    __m128i v_c = _mm_set1_epi32(0x5A5A5A5A);

    for (int r = 1; r <= 8; r++) {
        // 1. SubBytes trên 4 khối song song
        uint8_t s0[16], s1[16], s2[16], s3[16];
        _mm_storeu_si128((__m128i*)s0, b0); SUBBYTES_BLOCK(s0); b0 = _mm_loadu_si128((const __m128i*)s0);
        _mm_storeu_si128((__m128i*)s1, b1); SUBBYTES_BLOCK(s1); b1 = _mm_loadu_si128((const __m128i*)s1);
        _mm_storeu_si128((__m128i*)s2, b2); SUBBYTES_BLOCK(s2); b2 = _mm_loadu_si128((const __m128i*)s2);
        _mm_storeu_si128((__m128i*)s3, b3); SUBBYTES_BLOCK(s3); b3 = _mm_loadu_si128((const __m128i*)s3);

        // 2. SO(4) Permutation bằng lệnh _mm_shuffle_epi8 (1 chu kỳ clock cho mỗi khối)
        __m128i v_lut = _mm_loadu_si128((const __m128i*)ctx->perm_lut[r]);
        b0 = _mm_shuffle_epi8(b0, v_lut);
        b1 = _mm_shuffle_epi8(b1, v_lut);
        b2 = _mm_shuffle_epi8(b2, v_lut);
        b3 = _mm_shuffle_epi8(b3, v_lut);

        // 3. Chuyển vị ma trận 4x4 (Transpose) để gom các từ 32-bit tương ứng vào cùng 1 thanh ghi vector
        __m128 f0 = _mm_castsi128_ps(b0);
        __m128 f1 = _mm_castsi128_ps(b1);
        __m128 f2 = _mm_castsi128_ps(b2);
        __m128 f3 = _mm_castsi128_ps(b3);
        _MM_TRANSPOSE4_PS(f0, f1, f2, f3);

        b0 = _mm_castps_si128(f0); // chứa W0 của cả 4 khối
        b1 = _mm_castps_si128(f1); // chứa W1 của cả 4 khối
        b2 = _mm_castps_si128(f2); // chứa W2 của cả 4 khối
        b3 = _mm_castps_si128(f3); // chứa W3 của cả 4 khối

        // Tầng ARX Vector song song trên cả 4 khối
        __m128i t0 = _mm_add_epi32(b0, v_c);
        b1 = _mm_xor_si128(b1, _mm_or_si128(_mm_slli_epi32(t0, 7), _mm_srli_epi32(t0, 25)));

        __m128i t1 = _mm_add_epi32(b1, v_c);
        b2 = _mm_xor_si128(b2, _mm_or_si128(_mm_slli_epi32(t1, 11), _mm_srli_epi32(t1, 21)));

        __m128i t2 = _mm_add_epi32(b2, v_c);
        b3 = _mm_xor_si128(b3, _mm_or_si128(_mm_slli_epi32(t2, 13), _mm_srli_epi32(t2, 19)));

        __m128i t3 = _mm_add_epi32(b3, v_c);
        b0 = _mm_xor_si128(b0, _mm_or_si128(_mm_slli_epi32(t3, 17), _mm_srli_epi32(t3, 15)));

        // Chuyển vị ngược lại để phục hồi 4 khối 128-bit
        f0 = _mm_castsi128_ps(b0);
        f1 = _mm_castsi128_ps(b1);
        f2 = _mm_castsi128_ps(b2);
        f3 = _mm_castsi128_ps(b3);
        _MM_TRANSPOSE4_PS(f0, f1, f2, f3);

        b0 = _mm_castps_si128(f0);
        b1 = _mm_castps_si128(f1);
        b2 = _mm_castps_si128(f2);
        b3 = _mm_castps_si128(f3);

        // 4. AddRoundKey
        __m128i rkr = _mm_loadu_si128((const __m128i*)ctx->round_keys[r]);
        b0 = _mm_xor_si128(b0, rkr);
        b1 = _mm_xor_si128(b1, rkr);
        b2 = _mm_xor_si128(b2, rkr);
        b3 = _mm_xor_si128(b3, rkr);
    }

    _mm_storeu_si128((__m128i*)out0, b0);
    _mm_storeu_si128((__m128i*)out1, b1);
    _mm_storeu_si128((__m128i*)out2, b2);
    _mm_storeu_si128((__m128i*)out3, b3);
}

// Mã hóa chế độ Counter (CTR) đa khối song song AVX2 (4 khối = 64 bytes / bước lặp)
void rubik4d_ctr_encrypt_avx2(const rubik4d_ctx *ctx, const uint8_t *in, uint8_t *out, size_t len, const uint8_t iv[16]) {
    uint8_t ctr[16];
    memcpy(ctr, iv, 16);

    size_t offset = 0;

    // Vòng lặp chính xử lý 4 khối (64 bytes) cùng lúc
    while (offset + 64 <= len) {
        uint8_t c0[16], c1[16], c2[16], c3[16];
        memcpy(c0, ctr, 16); inc_counter_32(ctr);
        memcpy(c1, ctr, 16); inc_counter_32(ctr);
        memcpy(c2, ctr, 16); inc_counter_32(ctr);
        memcpy(c3, ctr, 16); inc_counter_32(ctr);

        uint8_t ks0[16], ks1[16], ks2[16], ks3[16];
        rubik4d_encrypt_4blocks_simd(ctx, c0, c1, c2, c3, ks0, ks1, ks2, ks3);

        // Vectorized XOR với plaintext
        __m128i p0 = _mm_loadu_si128((const __m128i*)(in + offset));
        __m128i p1 = _mm_loadu_si128((const __m128i*)(in + offset + 16));
        __m128i p2 = _mm_loadu_si128((const __m128i*)(in + offset + 32));
        __m128i p3 = _mm_loadu_si128((const __m128i*)(in + offset + 48));

        _mm_storeu_si128((__m128i*)(out + offset),      _mm_xor_si128(p0, _mm_loadu_si128((const __m128i*)ks0)));
        _mm_storeu_si128((__m128i*)(out + offset + 16), _mm_xor_si128(p1, _mm_loadu_si128((const __m128i*)ks1)));
        _mm_storeu_si128((__m128i*)(out + offset + 32), _mm_xor_si128(p2, _mm_loadu_si128((const __m128i*)ks2)));
        _mm_storeu_si128((__m128i*)(out + offset + 48), _mm_xor_si128(p3, _mm_loadu_si128((const __m128i*)ks3)));

        offset += 64;
    }

    // Xử lý các khối còn lại (dưới 64 bytes)
    while (offset < len) {
        uint8_t ks[16];
        rubik4d_encrypt_block_fast(ctx, ctr, ks);
        inc_counter_32(ctr);

        size_t rem = len - offset;
        size_t chunk = (rem < 16) ? rem : 16;
        for (size_t j = 0; j < chunk; j++) {
            out[offset + j] = in[offset + j] ^ ks[j];
        }
        offset += chunk;
    }
}
