# 🧊 Rubik-4D Cipher
> A 128-bit Symmetric Lightweight Block Cipher based on 4D Tesseract Geometry & ARX Diffusion

---

## 📌 1. Tổng quan (Overview)

**Rubik-4D Cipher** là thuật toán mã hóa khối đối xứng hạng nhẹ (Lightweight Symmetric Block Cipher) được thiết kế cho các hệ thống nhúng, vi điều khiển IoT và mạch phần cứng chuyên dụng.

### Bài toán giải quyết:
* Các thuật toán như AES sử dụng phép nhân ma trận trên trường hữu hạn $GF(2^8)$ (`MixColumns`) gây tốn diện tích mạch bán dẫn (Gate Equivalents) và tiêu hao nhiều năng lượng.
* Mạng ARX thuần túy (như Speck/Simon) tuy nhẹ nhưng tốc độ khuếch tán dữ liệu (diffusion) giữa các nhánh bit khá chậm, đòi hỏi nhiều vòng lặp.

### Giải pháp của Rubik-4D:
* **Ánh xạ siêu lập phương 4D (Tesseract):** 128 bit (16 byte) được khớp chính xác vào 16 đỉnh của không gian nhị phân 4 chiều $2 \times 2 \times 2 \times 2$.
* **Hoán vị nhóm quay $SO(4)$ tốc độ $O(1)$:** 12 phép quay trực giao được tính trước thành bảng tra cứu, giúp xáo trộn vị trí byte tức thì mà không tốn tài nguyên tính toán.
* **Tầng khuếch tán cộng dồn ARX:** Tận dụng hiện tượng dội sóng bit nhớ (carry propagation) của phép cộng modulo-256 để lan truyền biến động sang các byte lân cận.

---

## 📐 2. Kiến trúc giải thuật (Architecture)

Thuật toán xử lý khối dữ liệu 128-bit qua **12 vòng lặp (rounds)** theo sơ đồ:

```text
[Plaintext 128-bit] ──> ⊕ AddRoundKey(0)
                             │
       ┌─────────────────────┴────────────────────┐
       │             VÒNG LẶP (ROUND 1 - 12)       │
       │                                          │
       │  1. S-Box phi tuyến (AES Substitution)   │
       │  2. Xoay siêu lập phương 4D (SO(4))      │
       │  3. Khuếch tán cộng dồn bit nhớ (ARX)    │
       │  4. Trộn khóa con (⊕ AddRoundKey)        │
       └─────────────────────┬────────────────────┘
                             │
                             ▼
                    [Ciphertext 128-bit]
```

### Chi tiết 4 tầng xử lý cốt lõi:

* **Tầng 1 - S-box phi tuyến (Confusion):**
  * Từng byte dữ liệu được thay thế qua bảng S-box tiêu chuẩn để bẻ gãy tính tuyến tính của dữ liệu gốc.
  * `State[i] = SBOX[State[i]]` với $0 \le i < 16$.

* **Tầng 2 - Ánh xạ tọa độ & Xoay 4D động (Diffusion qua vị trí):**
  * Mỗi chỉ số byte $i$ được bóc tách thành 4 bit tọa độ $(x, y, z, w) \in \{0, 1\}^4$:
    * $x = (i \gg 3) \ \& \ 1$
    * $y = (i \gg 2) \ \& \ 1$
    * $z = (i \gg 1) \ \& \ 1$
    * $w = i \ \& \ 1$
  * Thực hiện phép quay 90 độ trên một trong 6 mặt phẳng trực giao ($XY, XZ, XW, YZ, YW, ZW$).
  * Khóa con của vòng trực tiếp điều khiển trục xoay và chiều xoay:
    * `Mặt phẳng = (Round + (KeyByte & 7)) % 6`
    * `Chiều quay = (KeyByte >> 3) & 1`
  * Dữ liệu được trích xuất qua 12 bảng hoán vị tính sẵn (`PERM_TABLES`) đạt tốc độ $O(1)$.

* **Tầng 3 - Khuếch tán liên byte ARX (Inter-byte Diffusion):**
  * Dùng phép cộng modulo kết hợp bitwise XOR để kích hoạt hiệu ứng thác lũ:
    * `State[(i + 1) % 16] = State[(i + 1) % 16] ^ ((State[i] + 0x5A) & 0xFF)`

* **Tầng 4 - Trộn khóa con (AddRoundKey):**
  * XOR trạng thái trung gian với subkey tương ứng của vòng đó.

---

## 📊 3. Bảng thông số & So sánh

| Tiêu chí | AES-128 | Speck / Simon | **Rubik-4D (Đề xuất)** |
| :--- | :---: | :---: | :---: |
| **Kích thước khối (Block size)** | 128 bits | 64 / 128 bits | **128 bits** |
| **Độ dài khóa (Key size)** | 128 bits | 128 bits | **128 bits** |
| **Cấu trúc mạng** | SPN thuần | Feistel / ARX | **SPN kết hợp ARX & Hình học 4D** |
| **Cơ chế hoán vị (P-box)** | `ShiftRows` cố định | Dịch bit tuần hoàn | **Xoay Tesseract 4D động theo khóa** |
| **Độ phức tạp tầng P-box** | Nhẹ | Cực nhẹ | **$O(1)$ Lookup Table** |
| **Shannon Entropy** | ~ 7.999 | ~ 7.998 | **~ 7.99** |

---

## 🚀 4. Hướng dẫn sử dụng (Quick Start)

### Cài đặt
```bash
git clone [https://github.com/toilacobemongmo/Rubic4DCrypto.git](https://github.com/toilacobemongmo/Rubic4DCrypto.git)
cd Rubic4DCrypto
```

### Code mẫu JavaScript
```javascript
const text = "Dữ liệu thử nghiệm IoT";
const password = "MatKhauBaoMat128Bit";

// 1. Mã hóa
const ciphertextHex = Rubik4DCrypto.encrypt(text, password, 12);
console.log("Ciphertext (Hex):", ciphertextHex);

// 2. Giải mã
const decryptedText = Rubik4DCrypto.decrypt(ciphertextHex, password, 12);
console.log("Decrypted (Plaintext):", decryptedText);
```

---

## 🛠 5. Kế hoạch nghiên cứu & Nâng cấp (Roadmap)

- [x] Xây dựng bản mẫu JavaScript hoàn chỉnh với 12 bảng hoán vị Rubik-4D tối ưu $O(1)$.
- [ ] **Nâng cấp Key Schedule:** Thay thế bộ sinh đồng dư tuyến tính (LCG) bằng cấu trúc sinh khóa phi tuyến kháng Related-Key Attack.
- [ ] **Chứng minh an toàn bằng MILP / SAT Solver:** Mô hình hóa toán học hệ phương trình vi sai/tuyến tính để xác định số hộp Active S-boxes tối thiểu.
- [ ] **Benchmark phần cứng thực tế:** Viết lại thuật toán bằng C/Assembly trên vi điều khiển ARM Cortex-M và nạp code Verilog lên FPGA để đo Cycles/Byte (cpb) và diện tích mạch (Gate Equivalents).

---

## 📄 6. Giấy phép & Tuyên bố học thuật

* Dự án được phân phối dưới giấy phép **MIT License**.
* Thiết kế tuân thủ nghiêm ngặt **Nguyên lý Kerckhoffs**: Độ an toàn của hệ thống hoàn toàn dựa vào tính bí mật của khóa, không phụ thuộc vào việc che giấu thuật toán.
