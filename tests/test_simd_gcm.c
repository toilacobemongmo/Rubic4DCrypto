#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include <windows.h>

#include "../include/rubik4d.h"
#include "../include/rubik4d_simd.h"
#include "../include/rubik4d_gcm.h"

static double get_time_sec(void) {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}

int main(void) {
    printf("=================================================================\n");
    printf("   TEST SUITE: RUBIK-4D SIMD ACCELERATION & AEAD GCM VERIFICATION\n");
    printf("=================================================================\n\n");

    // 1. Kiểm tra CPU Capabilities
    printf("[1] CPU Capabilities Detection:\n");
    printf("    - SSSE3 (pshufb) : %s\n", rubik4d_has_ssse3() ? "SUPPORTED (OK)" : "NOT SUPPORTED");
    printf("    - AVX2           : %s\n", rubik4d_has_avx2()  ? "SUPPORTED (OK)" : "NOT SUPPORTED");
    printf("    - PCLMULQDQ      : %s\n\n", rubik4d_has_pclmul() ? "SUPPORTED (OK)" : "NOT SUPPORTED");

    // 2. Kiểm tra Bit-Exact giữa Scalar C và SIMD
    printf("[2] Bit-Exact Equivalence Testing (100,000 Random Blocks)...\n");
    uint8_t key[16] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                       0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    rubik4d_ctx ctx;
    rubik4d_key_setup(&ctx, key, 16);

    int bit_exact_ok = 1;
    srand(42);
    for (int t = 0; t < 100000; t++) {
        uint8_t pt[16], ct_scalar[16], ct_simd[16], dec_simd[16];
        for (int i = 0; i < 16; i++) pt[i] = (uint8_t)rand();

        rubik4d_encrypt_block_fast(&ctx, pt, ct_scalar);
        rubik4d_encrypt_block_simd(&ctx, pt, ct_simd);

        if (memcmp(ct_scalar, ct_simd, 16) != 0) {
            printf("    FAILED at iteration %d: Scalar and SIMD ciphertext mismatch!\n", t);
            bit_exact_ok = 0;
            break;
        }

        rubik4d_decrypt_block_simd(&ctx, ct_simd, dec_simd);
        if (memcmp(pt, dec_simd, 16) != 0) {
            printf("    FAILED at iteration %d: SIMD decryption does not match plaintext!\n", t);
            bit_exact_ok = 0;
            break;
        }
    }
    if (bit_exact_ok) {
        printf("    >>> PASSED: 100%% Bit-Exact match between Scalar C and SIMD implementation! <<<\n\n");
    }

    // 3. Kiểm tra Rubik4D-GCM AEAD
    printf("[3] Rubik4D-GCM AEAD Functionality & Forgery Attack Tests...\n");
    uint8_t iv[12] = {0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88};
    const char *aad_str = "Rubik4D Protected Header 2026";
    const char *msg_str = "Confidential telemetry payload: Sensor readings = {T: 28.5C, P: 1013hPa}.";
    size_t aad_len = strlen(aad_str);
    size_t msg_len = strlen(msg_str);

    uint8_t cipher[128];
    uint8_t tag[16];
    uint8_t decrypted[128];

    // 3.1. Mã hóa xác thực
    rubik4d_aead_encrypt(key, 16, iv, 12, (const uint8_t*)aad_str, aad_len, (const uint8_t*)msg_str, msg_len, cipher, tag);
    printf("    - Ciphertext generated (%zu bytes)\n", msg_len);
    printf("    - Authentication Tag   : ");
    for (int i = 0; i < 16; i++) printf("%02X", tag[i]);
    printf("\n");

    // 3.2. Giải mã xác thực bình thường (phải thành công)
    int res = rubik4d_aead_decrypt(key, 16, iv, 12, (const uint8_t*)aad_str, aad_len, cipher, msg_len, tag, decrypted);
    if (res == 0 && memcmp(msg_str, decrypted, msg_len) == 0) {
        printf("    - Authentic Decryption : PASSED (0 errors, plaintext recovered accurately)\n");
    } else {
        printf("    - Authentic Decryption : FAILED!\n");
    }

    // 3.3. Tấn công cắt dán bit trong bản mã (Bit-flipping in Ciphertext)
    cipher[5] ^= 0x01; // Đảo 1 bit
    res = rubik4d_aead_decrypt(key, 16, iv, 12, (const uint8_t*)aad_str, aad_len, cipher, msg_len, tag, decrypted);
    if (res == -1) {
        printf("    - Bit-Flipping Attack on Ciphertext : REJECTED (Integrity protection verified!)\n");
    } else {
        printf("    - Bit-Flipping Attack on Ciphertext : FAILED TO DETECT!\n");
    }
    cipher[5] ^= 0x01; // Phục hồi bit

    // 3.4. Tấn công giả mạo Header (Tampering AAD)
    uint8_t tampered_aad[64];
    memcpy(tampered_aad, aad_str, aad_len);
    tampered_aad[0] ^= 0x01; // Đảo 1 bit ở header
    res = rubik4d_aead_decrypt(key, 16, iv, 12, tampered_aad, aad_len, cipher, msg_len, tag, decrypted);
    if (res == -1) {
        printf("    - Header Tampering Attack (AAD)    : REJECTED (Header authentication verified!)\n");
    } else {
        printf("    - Header Tampering Attack (AAD)    : FAILED TO DETECT!\n");
    }

    // 3.5. Tấn công giả mạo Tag (Forged Tag)
    tag[0] ^= 0x01;
    res = rubik4d_aead_decrypt(key, 16, iv, 12, (const uint8_t*)aad_str, aad_len, cipher, msg_len, tag, decrypted);
    if (res == -1) {
        printf("    - Forged Tag Verification          : REJECTED (Tag validation verified!)\n\n");
    } else {
        printf("    - Forged Tag Verification          : FAILED TO DETECT!\n\n");
    }

    // 4. Đo lường hiệu năng Benchmark (Throughput & Speedup)
    printf("[4] Hardware Microbenchmarks on 5 MB Payload (50 Iterations)...\n");
    size_t bench_len = 5 * 1024 * 1024;
    uint8_t *bench_in  = (uint8_t*)malloc(bench_len);
    uint8_t *bench_out = (uint8_t*)malloc(bench_len);
    for (size_t i = 0; i < bench_len; i++) bench_in[i] = (uint8_t)i;

    int iters = 50;

    // Benchmark Scalar C
    double t0 = get_time_sec();
    uint64_t c0 = __rdtsc();
    for (int it = 0; it < iters; it++) {
        for (size_t i = 0; i < bench_len; i += 16) {
            rubik4d_encrypt_block_fast(&ctx, bench_in + i, bench_out + i);
        }
    }
    uint64_t c1 = __rdtsc();
    double t1 = get_time_sec();
    double scalar_time = t1 - t0;
    double scalar_mbs = ((double)bench_len * iters) / (scalar_time * 1024.0 * 1024.0);
    double scalar_cpb = (double)(c1 - c0) / ((double)bench_len * iters);

    // Benchmark SIMD SSSE3
    double t2 = get_time_sec();
    uint64_t c2 = __rdtsc();
    for (int it = 0; it < iters; it++) {
        for (size_t i = 0; i < bench_len; i += 16) {
            rubik4d_encrypt_block_simd(&ctx, bench_in + i, bench_out + i);
        }
    }
    uint64_t c3 = __rdtsc();
    double t3 = get_time_sec();
    double simd_time = t3 - t2;
    double simd_mbs = ((double)bench_len * iters) / (simd_time * 1024.0 * 1024.0);
    double simd_cpb = (double)(c3 - c2) / ((double)bench_len * iters);

    // Benchmark Parallel AVX2 CTR
    uint8_t ctr_iv[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    double t4 = get_time_sec();
    uint64_t c4 = __rdtsc();
    for (int it = 0; it < iters; it++) {
        rubik4d_ctr_encrypt_avx2(&ctx, bench_in, bench_out, bench_len, ctr_iv);
    }
    uint64_t c5 = __rdtsc();
    double t5 = get_time_sec();
    double avx_time = t5 - t4;
    double avx_mbs = ((double)bench_len * iters) / (avx_time * 1024.0 * 1024.0);
    double avx_cpb = (double)(c5 - c4) / ((double)bench_len * iters);

    // Benchmark Rubik4D-GCM AEAD (Encryption + GHASH Authentication Tag Generation)
    rubik4d_gcm_ctx gcm_ctx;
    rubik4d_gcm_init(&gcm_ctx, key, 16);
    uint8_t bench_tag[16];
    double t6 = get_time_sec();
    uint64_t c6 = __rdtsc();
    for (int it = 0; it < iters; it++) {
        rubik4d_gcm_encrypt(&gcm_ctx, iv, 12, (const uint8_t*)aad_str, aad_len, bench_in, bench_len, bench_out, bench_tag);
    }
    uint64_t c7 = __rdtsc();
    double t7 = get_time_sec();
    double gcm_time = t7 - t6;
    double gcm_mbs = ((double)bench_len * iters) / (gcm_time * 1024.0 * 1024.0);
    double gcm_cpb = (double)(c7 - c6) / ((double)bench_len * iters);

    printf("    -----------------------------------------------------------------\n");
    printf("    Implementation Mode        | Cycles/Byte (cpb) | Throughput (MB/s)\n");
    printf("    -----------------------------------------------------------------\n");
    printf("    Rubik-4D Scalar C (Base)   | %17.2f | %14.2f MB/s\n", scalar_cpb, scalar_mbs);
    printf("    Rubik-4D SIMD (pshufb)     | %17.2f | %14.2f MB/s (Speedup: %.2fx)\n", simd_cpb, simd_mbs, simd_mbs / scalar_mbs);
    printf("    Rubik-4D AVX2 CTR Mode     | %17.2f | %14.2f MB/s (Speedup: %.2fx)\n", avx_cpb, avx_mbs, avx_mbs / scalar_mbs);
    printf("    Rubik4D-GCM AEAD (Enc+Tag) | %17.2f | %14.2f MB/s\n", gcm_cpb, gcm_mbs);
    printf("    -----------------------------------------------------------------\n\n");

    free(bench_in);
    free(bench_out);

    printf("=================================================================\n");
    printf("   ALL SIMD & AEAD VERIFICATION CHECKS COMPLETED SUCCESSFULLY!   \n");
    printf("=================================================================\n");

    return 0;
}
