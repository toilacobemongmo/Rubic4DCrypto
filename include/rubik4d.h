#ifndef RUBIK4D_H
#define RUBIK4D_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Định nghĩa các tham số mật mã cơ sở của Rubik-4D
#define RUBIK4D_BLOCK_SIZE  16   // 128 bit = 16 byte
#define RUBIK4D_KEY_SIZE    16   // 128 bit master key
#define RUBIK4D_ROUNDS      8    // Cấu trúc 8 vòng mã hóa
#define RUBIK4D_NUM_KEYS    9    // Khóa tiền trắng hóa K0 + 8 khóa vòng (K1..K8)
#define RUBIK4D_IV_SIZE     16   // Kích thước vector khởi tạo CBC

/**
 * @brief Khởi tạo các bảng hoán vị tĩnh SO(4) và S-Box nghịch đảo.
 * Cần gọi một lần trước khi thực hiện mã hóa/giải mã.
 */
void rubik4d_init_tables(void);

/**
 * @brief Sinh 9 khóa vòng 128-bit từ khóa chính (Master Key).
 * @param key Con trỏ tới mảng khóa chính 16 byte.
 * @param key_len Độ dài khóa (mặc định 16 byte).
 * @param round_keys Mảng đầu ra chứa 9 khóa vòng [9][16].
 */
void rubik4d_generate_round_keys(const uint8_t *key, size_t key_len, uint8_t round_keys[RUBIK4D_NUM_KEYS][RUBIK4D_BLOCK_SIZE]);

/**
 * @brief Mã hóa một khối đơn lẻ 16-byte (ECB primitive) qua 8 vòng.
 */
void rubik4d_encrypt_block(const uint8_t in[RUBIK4D_BLOCK_SIZE], 
                           uint8_t out[RUBIK4D_BLOCK_SIZE], 
                           const uint8_t round_keys[RUBIK4D_NUM_KEYS][RUBIK4D_BLOCK_SIZE]);

/**
 * @brief Giải mã một khối đơn lẻ 16-byte (ECB primitive) qua 8 vòng.
 */
void rubik4d_decrypt_block(const uint8_t in[RUBIK4D_BLOCK_SIZE], 
                           uint8_t out[RUBIK4D_BLOCK_SIZE], 
                           const uint8_t round_keys[RUBIK4D_NUM_KEYS][RUBIK4D_BLOCK_SIZE]);

/**
 * @brief Mã hóa mảng dữ liệu tùy ý theo chế độ CBC với đệm PKCS#7.
 * @param in Dữ liệu bản rõ đầu vào.
 * @param in_len Chiều dài dữ liệu bản rõ (byte).
 * @param out Bộ đệm chứa bản mã đầu ra (cần cấp phát tối thiểu: in_len + 16 byte).
 * @param key Khóa bí mật 16 byte.
 * @param key_len Chiều dài khóa bí mật.
 * @param iv Vector khởi tạo 16 byte.
 * @return Tổng số byte của bản mã sau khi đệm (luôn là bội số của 16).
 */
size_t rubik4d_encrypt(const uint8_t *in, size_t in_len, uint8_t *out, 
                       const uint8_t *key, size_t key_len, const uint8_t *iv);

/**
 * @brief Giải mã mảng dữ liệu theo chế độ CBC và kiểm tra/loại bỏ đệm PKCS#7.
 * @param in Dữ liệu bản mã đầu vào (phải là bội số của 16 byte).
 * @param in_len Chiều dài dữ liệu bản mã.
 * @param out Bộ đệm chứa bản rõ sau giải mã.
 * @param key Khóa bí mật 16 byte.
 * @param key_len Chiều dài khóa bí mật.
 * @param iv Vector khởi tạo 16 byte.
 * @return Chiều dài thực tế của dữ liệu bản rõ (0 nếu lỗi padding hoặc sai chiều dài).
 */
size_t rubik4d_decrypt(const uint8_t *in, size_t in_len, uint8_t *out, 
                       const uint8_t *key, size_t key_len, const uint8_t *iv);

/**
 * @brief Tính toán chỉ số Shannon Entropy của một khối dữ liệu.
 * @return Giá trị entropy trên thang 0.0 - 8.0 (tiệm cận 8.0 đối với bản mã ngẫu nhiên lý tưởng).
 */
double rubik4d_calculate_entropy(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // RUBIK4D_H