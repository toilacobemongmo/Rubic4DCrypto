#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "rubik4d.h"

/*
 * ============================================================================
 * NIST SP 800-22 & GM/T 0005-2021 Randomness Evaluation Data Generator
 * Target Cipher: Rubik-4D (128-bit block, 8 rounds, SO(4) & ARX diffusion)
 * ============================================================================
 * Configuration:
 *   - 1,000 sets  -> 125 MB  (1,000,000,000 bits) -> Recommended standard
 *   - 10,000 sets -> 1.25 GB (10,000,000,000 bits) -> Comprehensive battery
 */
#define NUM_SETS        10000ULL
#define BYTES_PER_SET   125000ULL            /* 1,000,000 bits per sequence */
#define TOTAL_BYTES     (NUM_SETS * BYTES_PER_SET)
#define CHUNK_SIZE      (1024 * 1024)        /* 1 MB buffer to minimize RAM */

int main(void) {
    /* 128-bit master key */
    const uint8_t master_key[16] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
    };

    /* Base Initialization Vector */
    const uint8_t base_iv[16] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    };

    /* Initialize Rubik-4D context (precomputes SO(4) permutation LUTs) */
    rubik4d_ctx ctx;
    rubik4d_key_setup(&ctx, master_key, 16);

    FILE *out_file = fopen("rubik_data.bin", "wb");
    if (!out_file) {
        fprintf(stderr, "[Error] Failed to open rubik_data.bin for writing.\n");
        return 1;
    }

    /* Allocate streaming I/O buffers */
    uint8_t *plain_chunk = (uint8_t *)calloc(CHUNK_SIZE, sizeof(uint8_t));
    uint8_t *cipher_chunk = (uint8_t *)malloc(CHUNK_SIZE);

    if (!plain_chunk || !cipher_chunk) {
        fprintf(stderr, "[Error] Memory allocation failed for 1 MB chunk buffer.\n");
        if (plain_chunk) free(plain_chunk);
        if (cipher_chunk) free(cipher_chunk);
        fclose(out_file);
        return 1;
    }

    printf("====================================================================\n");
    printf(" Generating Rubik-4D Pseudorandom Keystream (CBC Mode)\n");
    printf(" Target Sequences : %llu sets (1,000,000 bits per set)\n", NUM_SETS);
    printf(" Total Stream Size: %llu bytes (~%.2f MB / %.2f GB)\n", 
           TOTAL_BYTES, 
           (double)TOTAL_BYTES / (1024.0 * 1024.0),
           (double)TOTAL_BYTES / (1024.0 * 1024.0 * 1024.0));
    printf("====================================================================\n");

    size_t total_written = 0;
    uint8_t current_iv[16];
    memcpy(current_iv, base_iv, 16);

    while (total_written < TOTAL_BYTES) {
        size_t bytes_to_process = CHUNK_SIZE;
        if (total_written + bytes_to_process > TOTAL_BYTES) {
            bytes_to_process = TOTAL_BYTES - total_written;
        }

        /* 
         * CBC encryption loop utilizing rubik4d_encrypt_block_fast.
         * Refresh IV per 125,000-byte boundary to prevent long-period correlation bias.
         */
        for (size_t offset = 0; offset < bytes_to_process; offset += 16) {
            size_t global_byte_idx = total_written + offset;

            /* Set-boundary IV re-derivation (every 125,000 bytes) */
            if (global_byte_idx % BYTES_PER_SET == 0) {
                uint64_t set_idx = global_byte_idx / BYTES_PER_SET;
                for (int i = 0; i < 16; i++) {
                    current_iv[i] = base_iv[i] ^ (uint8_t)((set_idx >> ((i % 8) * 8)) & 0xFF);
                }
            }

            uint8_t block_in[16];
            uint8_t block_out[16];

            /* CBC Mode: XOR plaintext block with the preceding ciphertext block / IV */
            for (int i = 0; i < 16; i++) {
                block_in[i] = plain_chunk[offset + i] ^ current_iv[i];
            }

            /* Fast block encryption core */
            rubik4d_encrypt_block_fast(&ctx, block_in, block_out);

            /* Store ciphertext block and update IV for next iteration */
            memcpy(&cipher_chunk[offset], block_out, 16);
            memcpy(current_iv, block_out, 16);
        }

        /* Write encrypted chunk to file */
        size_t written_now = fwrite(cipher_chunk, 1, bytes_to_process, out_file);
        if (written_now != bytes_to_process) {
            fprintf(stderr, "\n[Error] Disk write failure.\n");
            break;
        }
        total_written += written_now;

        /* Real-time progress display */
        printf("\r[Progress]: %6.2f%% (%zu / %llu bytes)", 
               (double)total_written * 100.0 / TOTAL_BYTES, total_written, TOTAL_BYTES);
        fflush(stdout);
    }

    printf("\n\n[Success] Binary keystream saved to rubik_data.bin (%zu bytes).\n", total_written);

    fclose(out_file);
    free(plain_chunk);
    free(cipher_chunk);
    return 0;
}