#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "rubik4d.h"

#define NUM_SAMPLES 10000

#define ROTL32(v, n) (((v) << (n)) | ((v) >> (32 - (n))))

// AES S-box chuẩn FIPS-197
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

// Hàm mã hóa từng vòng Rubik-4D độc lập lưu lại trạng thái sau mỗi vòng
static void rubik4d_trace_rounds(const rubik4d_ctx *ctx, const uint8_t in[16], uint8_t round_states[9][16]) {
    uint8_t state[16];
    // Vòng 0: Key Whitening
    for (int i = 0; i < 16; i++) {
        state[i] = in[i] ^ ctx->round_keys[0][i];
    }
    memcpy(round_states[0], state, 16);

    for (int r = 1; r <= 8; r++) {
        // 1. SubBytes
        for (int i = 0; i < 16; i++) {
            state[i] = AES_SBOX[state[i]];
        }

        // 2. SO(4) Permutation
        const uint8_t* lut = ctx->perm_lut[r];
        uint8_t rot[16];
        for (int i = 0; i < 16; i++) {
            rot[i] = state[lut[i]];
        }

        // 3. ARX Ripple 32-bit
        uint32_t* w = (uint32_t*)rot;
        w[1] ^= ROTL32(w[0] + 0x5A5A5A5AU, 7);
        w[2] ^= ROTL32(w[1] + 0x5A5A5A5AU, 11);
        w[3] ^= ROTL32(w[2] + 0x5A5A5A5AU, 13);
        w[0] ^= ROTL32(w[3] + 0x5A5A5A5AU, 17);

        // 4. AddRoundKey
        for (int i = 0; i < 16; i++) {
            state[i] = rot[i] ^ ctx->round_keys[r][i];
        }

        memcpy(round_states[r], state, 16);
    }
}

// Xoroshiro128+ PRNG
typedef struct {
    uint64_t s[2];
} xoroshiro_ctx;

static inline uint64_t rotl64(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

static uint64_t xoroshiro_next(xoroshiro_ctx *ctx) {
    const uint64_t s0 = ctx->s[0];
    uint64_t s1 = ctx->s[1];
    const uint64_t result = rotl64(s0 + s1, 17) + s0;
    s1 ^= s0;
    ctx->s[0] = rotl64(s0, 49) ^ s1 ^ (s1 << 21);
    ctx->s[1] = rotl64(s1, 28);
    return result;
}

static void fill_random(xoroshiro_ctx *ctx, uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i += 8) {
        uint64_t r = xoroshiro_next(ctx);
        size_t chunk = (len - i < 8) ? (len - i) : 8;
        memcpy(buf + i, &r, chunk);
    }
}

static inline int count_bit_diff(const uint8_t *a, const uint8_t *b, size_t len) {
    int diff = 0;
    for (size_t i = 0; i < len; i++) {
        diff += __builtin_popcount(a[i] ^ b[i]);
    }
    return diff;
}

// ============================================================================
// 1. Plaintext Avalanche Effect (Đo theo từng vòng từ 1 đến 8)
// ============================================================================
void run_plaintext_avalanche_test(void) {
    printf("====================================================================\n");
    printf(" 1. PLAINTEXT AVALANCHE EFFECT (ROUND-BY-ROUND 1 TO 8, 10,000 SAMPLES)\n");
    printf("====================================================================\n");

    xoroshiro_ctx rng = { .s = { 0xA5A5A5A512345678ULL, 0x5A5A5A5A87654321ULL } };
    rubik4d_init_tables();

    double sum_diff[9] = {0.0};
    double sum_diff_sq[9] = {0.0};
    int min_diff[9], max_diff[9];
    for (int r = 0; r <= 8; r++) {
        min_diff[r] = 128;
        max_diff[r] = 0;
    }

    // Kiểm tra độ tương thích của rubik4d_trace_rounds với rubik4d_encrypt_block_fast
    {
        uint8_t test_k[16], test_p[16], test_c_fast[16], test_states[9][16];
        fill_random(&rng, test_k, 16);
        fill_random(&rng, test_p, 16);
        rubik4d_ctx ctx;
        rubik4d_key_setup(&ctx, test_k, 16);
        rubik4d_encrypt_block_fast(&ctx, test_p, test_c_fast);
        rubik4d_trace_rounds(&ctx, test_p, test_states);
        if (memcmp(test_c_fast, test_states[8], 16) != 0) {
            fprintf(stderr, "[FATAL ERROR] Round tracer does not match rubik4d_encrypt_block_fast!\n");
            exit(1);
        }
    }

    for (int sample = 0; sample < NUM_SAMPLES; sample++) {
        uint8_t key[16], p1[16], p2[16];
        fill_random(&rng, key, 16);
        fill_random(&rng, p1, 16);
        memcpy(p2, p1, 16);

        // Lật đúng 1 bit ngẫu nhiên trong bản rõ
        int bit_idx = (int)(xoroshiro_next(&rng) % 128);
        p2[bit_idx / 8] ^= (1 << (bit_idx % 8));

        rubik4d_ctx ctx;
        rubik4d_key_setup(&ctx, key, 16);

        uint8_t states1[9][16], states2[9][16];
        rubik4d_trace_rounds(&ctx, p1, states1);
        rubik4d_trace_rounds(&ctx, p2, states2);

        for (int r = 0; r <= 8; r++) {
            int d = count_bit_diff(states1[r], states2[r], 16);
            sum_diff[r] += d;
            sum_diff_sq[r] += (double)d * d;
            if (d < min_diff[r]) min_diff[r] = d;
            if (d > max_diff[r]) max_diff[r] = d;
        }
    }

    printf("%-10s | %-14s | %-14s | %-12s | %-12s | %-14s\n",
           "Iteration", "Mean Diff Bits", "Bit Flip (%)", "Std Dev (%)", "Min / Max", "Status");
    printf("----------------------------------------------------------------------------------------\n");

    for (int r = 0; r <= 8; r++) {
        double mean_bits = sum_diff[r] / NUM_SAMPLES;
        double mean_pct = (mean_bits / 128.0) * 100.0;
        double variance = (sum_diff_sq[r] / NUM_SAMPLES) - (mean_bits * mean_bits);
        double std_dev_bits = sqrt(variance > 0 ? variance : 0);
        double std_dev_pct = (std_dev_bits / 128.0) * 100.0;

        char min_max_str[32];
        snprintf(min_max_str, sizeof(min_max_str), "%d / %d", min_diff[r], max_diff[r]);

        const char *status = "Diffusing";
        if (r == 0) status = "Whitening (1 bit)";
        else if (mean_pct >= 48.0 && mean_pct <= 52.0) status = "Full Avalanche";

        if (r == 0) {
            printf("Round 0    | %9.3f/128 | %11.4f%%  | %9.4f%%  | %-12s | %s\n",
                   mean_bits, mean_pct, std_dev_pct, min_max_str, status);
        } else {
            printf("Round %-4d | %9.3f/128 | %11.4f%%  | %9.4f%%  | %-12s | %s\n",
                   r, mean_bits, mean_pct, std_dev_pct, min_max_str, status);
        }
    }
    printf("----------------------------------------------------------------------------------------\n\n");
}

// ============================================================================
// 2. Key Sensitivity on Ciphertext (Độ nhạy Khóa trên Bản mã)
// ============================================================================
void run_key_sensitivity_test(void) {
    printf("====================================================================\n");
    printf(" 2. KEY SENSITIVITY ON CIPHERTEXT (10,000 SAMPLES)\n");
    printf("====================================================================\n");

    xoroshiro_ctx rng = { .s = { 0xF0E1D2C3B4A59687ULL, 0x1A2B3C4D5E6F7081ULL } };
    rubik4d_init_tables();

    double sum_diff[9] = {0.0};
    double sum_diff_sq[9] = {0.0};
    int min_diff[9], max_diff[9];
    for (int r = 0; r <= 8; r++) {
        min_diff[r] = 128;
        max_diff[r] = 0;
    }

    for (int sample = 0; sample < NUM_SAMPLES; sample++) {
        uint8_t pt[16], k1[16], k2[16];
        fill_random(&rng, pt, 16);
        fill_random(&rng, k1, 16);
        memcpy(k2, k1, 16);

        // Lật đúng 1 bit ngẫu nhiên trong Master Key
        int bit_idx = (int)(xoroshiro_next(&rng) % 128);
        k2[bit_idx / 8] ^= (1 << (bit_idx % 8));

        rubik4d_ctx ctx1, ctx2;
        rubik4d_key_setup(&ctx1, k1, 16);
        rubik4d_key_setup(&ctx2, k2, 16);

        uint8_t states1[9][16], states2[9][16];
        rubik4d_trace_rounds(&ctx1, pt, states1);
        rubik4d_trace_rounds(&ctx2, pt, states2);

        for (int r = 0; r <= 8; r++) {
            int d = count_bit_diff(states1[r], states2[r], 16);
            sum_diff[r] += d;
            sum_diff_sq[r] += (double)d * d;
            if (d < min_diff[r]) min_diff[r] = d;
            if (d > max_diff[r]) max_diff[r] = d;
        }
    }

    printf("%-10s | %-14s | %-14s | %-12s | %-12s | %-14s\n",
           "Iteration", "Mean Diff Bits", "Bit Flip (%)", "Std Dev (%)", "Min / Max", "Status");
    printf("----------------------------------------------------------------------------------------\n");

    for (int r = 0; r <= 8; r++) {
        double mean_bits = sum_diff[r] / NUM_SAMPLES;
        double mean_pct = (mean_bits / 128.0) * 100.0;
        double variance = (sum_diff_sq[r] / NUM_SAMPLES) - (mean_bits * mean_bits);
        double std_dev_bits = sqrt(variance > 0 ? variance : 0);
        double std_dev_pct = (std_dev_bits / 128.0) * 100.0;

        char min_max_str[32];
        snprintf(min_max_str, sizeof(min_max_str), "%d / %d", min_diff[r], max_diff[r]);

        const char *status = "Diffusing";
        if (r == 0) status = "Whitening (1 bit)";
        else if (mean_pct >= 48.0 && mean_pct <= 52.0) status = "Full Avalanche";

        if (r == 0) {
            printf("Round 0    | %9.3f/128 | %11.4f%%  | %9.4f%%  | %-12s | %s\n",
                   mean_bits, mean_pct, std_dev_pct, min_max_str, status);
        } else {
            printf("Round %-4d | %9.3f/128 | %11.4f%%  | %9.4f%%  | %-12s | %s\n",
                   r, mean_bits, mean_pct, std_dev_pct, min_max_str, status);
        }
    }

    double final_mean = sum_diff[8] / NUM_SAMPLES;
    double final_pct = (final_mean / 128.0) * 100.0;
    double final_var = (sum_diff_sq[8] / NUM_SAMPLES) - (final_mean * final_mean);
    double final_std = (sqrt(final_var > 0 ? final_var : 0) / 128.0) * 100.0;

    printf("----------------------------------------------------------------------------------------\n");
    printf("[+] Final Ciphertext Key Sensitivity: %.4f%% +/- %.4f%% (Min: %d, Max: %d bits)\n",
           final_pct, final_std, min_diff[8], max_diff[8]);
    printf("[+] Ideal Expectation: 50.0000%%\n\n");
}

static void export_json_results(void) {
    xoroshiro_ctx rng_pt = { .s = { 0xA5A5A5A512345678ULL, 0x5A5A5A5A87654321ULL } };
    xoroshiro_ctx rng_key = { .s = { 0xF0E1D2C3B4A59687ULL, 0x1A2B3C4D5E6F7081ULL } };
    rubik4d_init_tables();

    double pt_sum[9] = {0.0}, pt_sum_sq[9] = {0.0};
    int pt_min[9], pt_max[9];
    for (int r = 0; r <= 8; r++) { pt_min[r] = 128; pt_max[r] = 0; }

    for (int sample = 0; sample < NUM_SAMPLES; sample++) {
        uint8_t key[16], p1[16], p2[16];
        fill_random(&rng_pt, key, 16);
        fill_random(&rng_pt, p1, 16);
        memcpy(p2, p1, 16);
        int bit_idx = (int)(xoroshiro_next(&rng_pt) % 128);
        p2[bit_idx / 8] ^= (1 << (bit_idx % 8));

        rubik4d_ctx ctx;
        rubik4d_key_setup(&ctx, key, 16);
        uint8_t states1[9][16], states2[9][16];
        rubik4d_trace_rounds(&ctx, p1, states1);
        rubik4d_trace_rounds(&ctx, p2, states2);

        for (int r = 0; r <= 8; r++) {
            int d = count_bit_diff(states1[r], states2[r], 16);
            pt_sum[r] += d;
            pt_sum_sq[r] += (double)d * d;
            if (d < pt_min[r]) pt_min[r] = d;
            if (d > pt_max[r]) pt_max[r] = d;
        }
    }

    double key_sum[9] = {0.0}, key_sum_sq[9] = {0.0};
    int key_min[9], key_max[9];
    for (int r = 0; r <= 8; r++) { key_min[r] = 128; key_max[r] = 0; }

    for (int sample = 0; sample < NUM_SAMPLES; sample++) {
        uint8_t pt[16], k1[16], k2[16];
        fill_random(&rng_key, pt, 16);
        fill_random(&rng_key, k1, 16);
        memcpy(k2, k1, 16);
        int bit_idx = (int)(xoroshiro_next(&rng_key) % 128);
        k2[bit_idx / 8] ^= (1 << (bit_idx % 8));

        rubik4d_ctx ctx1, ctx2;
        rubik4d_key_setup(&ctx1, k1, 16);
        rubik4d_key_setup(&ctx2, k2, 16);
        uint8_t states1[9][16], states2[9][16];
        rubik4d_trace_rounds(&ctx1, pt, states1);
        rubik4d_trace_rounds(&ctx2, pt, states2);

        for (int r = 0; r <= 8; r++) {
            int d = count_bit_diff(states1[r], states2[r], 16);
            key_sum[r] += d;
            key_sum_sq[r] += (double)d * d;
            if (d < key_min[r]) key_min[r] = d;
            if (d > key_max[r]) key_max[r] = d;
        }
    }

    FILE *f = fopen("reports/sensitivity.json", "w");
    if (!f) f = fopen("sensitivity.json", "w");
    if (!f) return;

    fprintf(f, "{\n  \"pt_avalanche\": [\n");
    for (int r = 0; r <= 8; r++) {
        double m_bits = pt_sum[r] / NUM_SAMPLES;
        double m_pct = (m_bits / 128.0) * 100.0;
        double var = (pt_sum_sq[r] / NUM_SAMPLES) - (m_bits * m_bits);
        double s_pct = (sqrt(var > 0 ? var : 0) / 128.0) * 100.0;
        const char *st = (r == 0) ? "Whitening (1 bit)" : (m_pct >= 48.0 ? "Full Avalanche" : "Diffusing");
        fprintf(f, "    {\"round\": \"Round %d\", \"bits\": %.3f, \"pct\": %.4f, \"std\": %.4f, \"min\": %d, \"max\": %d, \"status\": \"%s\"}%s\n",
                r, m_bits, m_pct, s_pct, pt_min[r], pt_max[r], st, (r == 8) ? "" : ",");
    }
    fprintf(f, "  ],\n  \"key_sensitivity\": [\n");
    for (int r = 0; r <= 8; r++) {
        double m_bits = key_sum[r] / NUM_SAMPLES;
        double m_pct = (m_bits / 128.0) * 100.0;
        double var = (key_sum_sq[r] / NUM_SAMPLES) - (m_bits * m_bits);
        double s_pct = (sqrt(var > 0 ? var : 0) / 128.0) * 100.0;
        const char *st = (r == 0) ? "Whitening (1 bit)" : (m_pct >= 48.0 ? "Full Avalanche" : "Diffusing");
        fprintf(f, "    {\"round\": \"Round %d\", \"bits\": %.3f, \"pct\": %.4f, \"std\": %.4f, \"min\": %d, \"max\": %d, \"status\": \"%s\"}%s\n",
                r, m_bits, m_pct, s_pct, key_min[r], key_max[r], st, (r == 8) ? "" : ",");
    }
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

int main(void) {
    run_plaintext_avalanche_test();
    run_key_sensitivity_test();
    export_json_results();
    return 0;
}
