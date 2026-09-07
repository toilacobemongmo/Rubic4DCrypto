#include <stdio.h>
#include <stdlib.h>
#include "rubik4d.h"

int main() {
    size_t size = 12500000; // 12.5 MB = 100 triệu bit
    uint8_t *plain = calloc(size, 1); // Dữ liệu toàn 0x00 để thử thách độ xáo trộn
    uint8_t *cipher = malloc(size + 16);
    
    uint8_t key[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 
                       0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};
    uint8_t iv[16]  = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 
                       0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

    rubik4d_encrypt(plain, size, cipher, key, 16, iv);

    FILE *f = fopen("rubik_data.bin", "wb");
    fwrite(cipher, 1, size, f);
    fclose(f);

    printf("Tao file rubik_data.bin thanh cong!\n");
    free(plain); free(cipher);
    return 0;
}