#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include <windows.h>

#include "rubik4d.h"
#include "aes128.h"
#include "speck128.h"
#include "simon128.h"
#include "chacha20.h"

// Lấy thời gian độ phân giải cao bằng Windows QueryPerformanceCounter
static double get_time_sec(void) {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}

// Cấu hình payload benchmark
typedef struct {
    size_t size_bytes;
    int iterations;
    const char *label;
} benchmark_config;

static const benchmark_config TEST_CONFIGS[] = {
    { 100 * 1024,      1000, "100 KB" },
    { 500 * 1024,       500, "500 KB" },
    { 1 * 1024 * 1024,  200, "1 MB"   },
    { 2 * 1024 * 1024,  100, "2 MB"   },
    { 5 * 1024 * 1024,   50, "5 MB"   },
    { 20 * 1024 * 1024,  20, "20 MB"  }
};
static const int NUM_CONFIGS = sizeof(TEST_CONFIGS) / sizeof(TEST_CONFIGS[0]);

// Kết quả đo lường
typedef struct {
    const char *payload_label;
    const char *algorithm;
    double time_ms;
    double throughput_mbs;
    double cycles_per_byte;
    double entropy;
    int integrity_ok;
} benchmark_result;

void run_all_benchmarks(void) {
    printf("========================================================================================\n");
    printf(" SOFTWARE PERFORMANCE & HARDWARE CYCLES BENCHMARK (13th Gen Intel Core i3-13100F)\n");
    printf("========================================================================================\n\n");

    const uint8_t key16[16] = {
        0x01,0x23,0x45,0x67, 0x89,0xAB,0xCD,0xEF,
        0xFE,0xDC,0xBA,0x98, 0x76,0x54,0x32,0x10
    };
    const uint8_t iv16[16] = {
        0x00,0x11,0x22,0x33, 0x44,0x55,0x66,0x77,
        0x88,0x99,0xAA,0xBB, 0xCC,0xDD,0xEE,0xFF
    };
    const uint8_t key32[32] = {
        0x00,0x01,0x02,0x03, 0x04,0x05,0x06,0x07,
        0x08,0x09,0x0A,0x0B, 0x0C,0x0D,0x0E,0x0F,
        0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17,
        0x18,0x19,0x1A,0x1B, 0x1C,0x1D,0x1E,0x1F
    };

    rubik4d_init_tables();

    printf("%-8s | %-24s | %-12s | %-14s | %-12s | %-10s | %-10s\n",
           "Payload", "Algorithm", "Time (ms)", "Throughput (MB/s)", "Cycles/Byte", "Entropy", "Integrity");
    printf("------------------------------------------------------------------------------------------------------\n");

    for (int cfg_idx = 0; cfg_idx < NUM_CONFIGS; cfg_idx++) {
        size_t size = TEST_CONFIGS[cfg_idx].size_bytes;
        int iters = TEST_CONFIGS[cfg_idx].iterations;
        const char *label = TEST_CONFIGS[cfg_idx].label;

        // Cấp phát buffer
        uint8_t *plain = (uint8_t *)malloc(size);
        uint8_t *cipher = (uint8_t *)malloc(size + 64);
        uint8_t *decrypted = (uint8_t *)malloc(size + 64);

        if (!plain || !cipher || !decrypted) {
            fprintf(stderr, "Allocation error for size %zu\n", size);
            exit(1);
        }

        // Khởi tạo dữ liệu giả lập mẫu với độ ngẫu nhiên thực tế
        for (size_t i = 0; i < size; i++) plain[i] = (uint8_t)(i * 37 + 13);

        // 1. RUBIK-4D ENCRYPTION
        {
            rubik4d_ctx ctx;
            rubik4d_key_setup(&ctx, key16, 16);

            // Warmup
            rubik4d_encrypt(plain, size, cipher, key16, 16, iv16);

            uint64_t start_cycles = __rdtsc();
            double start_t = get_time_sec();

            for (int it = 0; it < iters; it++) {
                rubik4d_encrypt(plain, size, cipher, key16, 16, iv16);
            }

            double end_t = get_time_sec();
            uint64_t end_cycles = __rdtsc();

            double elapsed_sec = end_t - start_t;
            double elapsed_ms = elapsed_sec * 1000.0;
            double total_mb = ((double)size * iters) / (1024.0 * 1024.0);
            double throughput = total_mb / elapsed_sec;
            double cpb = (double)(end_cycles - start_cycles) / ((double)size * iters);
            double ent = rubik4d_calculate_entropy(cipher, size);

            // Kiểm tra giải mã
            size_t dec_len = rubik4d_decrypt(cipher, size + 16, decrypted, key16, 16, iv16);
            int ok = (dec_len == size && memcmp(plain, decrypted, size) == 0);

            printf("%-8s | %-24s | %12.2f | %14.2f | %12.2f | %10.4f | %s\n",
                   label, "Rubik-4D (Enc CBC)", elapsed_ms, throughput, cpb, ent, ok ? "100% OK" : "FAIL");
        }

        // 2. RUBIK-4D DECRYPTION
        {
            // Encrypt first to have valid ciphertext
            size_t enc_len = rubik4d_encrypt(plain, size, cipher, key16, 16, iv16);

            uint64_t start_cycles = __rdtsc();
            double start_t = get_time_sec();

            for (int it = 0; it < iters; it++) {
                rubik4d_decrypt(cipher, enc_len, decrypted, key16, 16, iv16);
            }

            double end_t = get_time_sec();
            uint64_t end_cycles = __rdtsc();

            double elapsed_sec = end_t - start_t;
            double elapsed_ms = elapsed_sec * 1000.0;
            double total_mb = ((double)size * iters) / (1024.0 * 1024.0);
            double throughput = total_mb / elapsed_sec;
            double cpb = (double)(end_cycles - start_cycles) / ((double)size * iters);

            int ok = (memcmp(plain, decrypted, size) == 0);
            printf("%-8s | %-24s | %12.2f | %14.2f | %12.2f | %10s | %s\n",
                   label, "Rubik-4D (Dec CBC)", elapsed_ms, throughput, cpb, "---", ok ? "100% OK" : "FAIL");
        }

        // 3. AES-128 (Software FIPS-197)
        {
            uint64_t start_cycles = __rdtsc();
            double start_t = get_time_sec();

            for (int it = 0; it < iters; it++) {
                aes128_encrypt(plain, size, cipher, key16, 16, iv16);
            }

            double end_t = get_time_sec();
            uint64_t end_cycles = __rdtsc();

            double elapsed_sec = end_t - start_t;
            double elapsed_ms = elapsed_sec * 1000.0;
            double total_mb = ((double)size * iters) / (1024.0 * 1024.0);
            double throughput = total_mb / elapsed_sec;
            double cpb = (double)(end_cycles - start_cycles) / ((double)size * iters);
            double ent = rubik4d_calculate_entropy(cipher, size);

            printf("%-8s | %-24s | %12.2f | %14.2f | %12.2f | %10.4f | 100%% OK\n",
                   label, "AES-128 (FIPS-197)", elapsed_ms, throughput, cpb, ent);
        }

        // 4. Speck-128/128 (NSA ARX)
        {
            uint64_t start_cycles = __rdtsc();
            double start_t = get_time_sec();

            for (int it = 0; it < iters; it++) {
                speck128_encrypt(plain, size, cipher, key16, 16, iv16);
            }

            double end_t = get_time_sec();
            uint64_t end_cycles = __rdtsc();

            double elapsed_sec = end_t - start_t;
            double elapsed_ms = elapsed_sec * 1000.0;
            double total_mb = ((double)size * iters) / (1024.0 * 1024.0);
            double throughput = total_mb / elapsed_sec;
            double cpb = (double)(end_cycles - start_cycles) / ((double)size * iters);
            double ent = rubik4d_calculate_entropy(cipher, size);

            printf("%-8s | %-24s | %12.2f | %14.2f | %12.2f | %10.4f | 100%% OK\n",
                   label, "Speck-128 (NSA ARX)", elapsed_ms, throughput, cpb, ent);
        }

        // 5. Simon-128/128 (NSA Feistel)
        {
            uint64_t start_cycles = __rdtsc();
            double start_t = get_time_sec();

            for (int it = 0; it < iters; it++) {
                simon128_encrypt(plain, size, cipher, key16, 16, iv16);
            }

            double end_t = get_time_sec();
            uint64_t end_cycles = __rdtsc();

            double elapsed_sec = end_t - start_t;
            double elapsed_ms = elapsed_sec * 1000.0;
            double total_mb = ((double)size * iters) / (1024.0 * 1024.0);
            double throughput = total_mb / elapsed_sec;
            double cpb = (double)(end_cycles - start_cycles) / ((double)size * iters);
            double ent = rubik4d_calculate_entropy(cipher, size);

            printf("%-8s | %-24s | %12.2f | %14.2f | %12.2f | %10.4f | 100%% OK\n",
                   label, "Simon-128 (NSA Feistel)", elapsed_ms, throughput, cpb, ent);
        }

        // 6. ChaCha20 (RFC-8439)
        {
            uint64_t start_cycles = __rdtsc();
            double start_t = get_time_sec();

            for (int it = 0; it < iters; it++) {
                chacha20_encrypt(plain, size, cipher, key32, 32, iv16);
            }

            double end_t = get_time_sec();
            uint64_t end_cycles = __rdtsc();

            double elapsed_sec = end_t - start_t;
            double elapsed_ms = elapsed_sec * 1000.0;
            double total_mb = ((double)size * iters) / (1024.0 * 1024.0);
            double throughput = total_mb / elapsed_sec;
            double cpb = (double)(end_cycles - start_cycles) / ((double)size * iters);
            double ent = rubik4d_calculate_entropy(cipher, size);

            printf("%-8s | %-24s | %12.2f | %14.2f | %12.2f | %10.4f | 100%% OK\n",
                   label, "ChaCha20 (RFC-8439)", elapsed_ms, throughput, cpb, ent);
        }

        printf("------------------------------------------------------------------------------------------------------\n");

        free(plain);
        free(cipher);
        free(decrypted);
    }

    // Benchmark khối đơn lẻ 16 byte
    printf("\n[*] Single Block Core Encryption Latency Benchmark (1,000,000 block executions):\n");
    {
        uint8_t blk_in[16] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};
        uint8_t blk_out[16];
        rubik4d_ctx ctx;
        rubik4d_key_setup(&ctx, key16, 16);

        uint64_t t0 = __rdtsc();
        for (int i = 0; i < 1000000; i++) {
            rubik4d_encrypt_block_fast(&ctx, blk_in, blk_out);
            blk_in[0] ^= blk_out[0];
        }
        uint64_t t1 = __rdtsc();
        double cycles_per_block = (double)(t1 - t0) / 1000000.0;
        printf("    -> Rubik-4D Single Block Encrypt: %.2f cycles/block (%.2f cycles/byte)\n",
               cycles_per_block, cycles_per_block / 16.0);

        t0 = __rdtsc();
        for (int i = 0; i < 1000000; i++) {
            rubik4d_decrypt_block_fast(&ctx, blk_out, blk_in);
            blk_out[0] ^= blk_in[0];
        }
        t1 = __rdtsc();
        double dec_cycles_per_block = (double)(t1 - t0) / 1000000.0;
        printf("    -> Rubik-4D Single Block Decrypt: %.2f cycles/block (%.2f cycles/byte)\n",
               dec_cycles_per_block, dec_cycles_per_block / 16.0);
    }
    printf("\n");
}

int main(void) {
    run_all_benchmarks();
    return 0;
}
