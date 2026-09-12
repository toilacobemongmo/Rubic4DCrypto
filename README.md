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
* **Hiệu suất cực đại với 6 vòng lặp (6 Rounds):** Tối ưu hóa điểm nghẽn, đẩy thông lượng phần mềm (Software Throughput) lên mức ~79 MB/s, vượt qua AES trên các bài test phần mềm thuần túy mà vẫn giữ nguyên độ an toàn.

---

## 📐 2. Kiến trúc giải thuật (Architecture)

Thuật toán xử lý khối dữ liệu 128-bit qua **6 vòng lặp (rounds)** theo sơ đồ:

```text
[Plaintext 128-bit] ──> ⊕ AddRoundKey(0)
                             │
       ┌─────────────────────┴────────────────────┐
       │             VÒNG LẶP (ROUND 1 - 6)       │
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

**Đặc tính bảo mật cốt lõi:**
* **Key Schedule SPN (Substitution-Permutation Network):** Lịch trình khóa phi tuyến tính mạnh mẽ, loại bỏ hoàn toàn điểm yếu của LCG, chống lại Related-Key Attack.
* **Chế độ vận hành CBC (Cipher Block Chaining):** Hỗ trợ chuẩn xác chế độ CBC với IV để đảm bảo an toàn cho các luồng dữ liệu lớn.

## 📊 3. Bảng thông số & So sánh

| Tiêu chí | AES-128 | Speck / Simon | Rubik-4D (Đề xuất) |
| :--- | :--- | :--- | :--- |
| **Kích thước khối** | 128 bits | 64 / 128 bits | 128 bits |
| **Độ dài khóa** | 128 bits | 128 bits | 128 bits |
| **Số vòng lặp (Rounds)** | 10 | 32 | 6 |
| **Cấu trúc mạng** | SPN thuần | Feistel / ARX | SPN kết hợp ARX & Hình học 4D |
| **Cơ chế hoán vị (P-box)** | ShiftRows cố định | Dịch bit tuần hoàn | Xoay Tesseract 4D động theo khóa |
| **Thông lượng (x86 SW)** | ~ 45 MB/s | Rất cao | ~ 79 MB/s |
| **NIST SP 800-22** | Passed | Passed | Passed (186/188 tests) |

## 🚀 4. Hướng dẫn sử dụng (Quick Start)

### 4.1. Tải mã nguồn
```bash
git clone https://github.com/toilacobemongmo/Rubic4DCrypto.git
cd Rubic4DCrypto
```

### 4.2. Biên dịch & Chạy GUI Benchmark (C++ / ImGui)
Ứng dụng Benchmark trực quan so sánh trực tiếp hiệu năng giữa Rubik-4D, AES và Speck:
```bash
g++ -O3 -std=c++17 src/main.cpp src/rubik4d.c src/aes128.c src/speck128.c src/analysis.cpp imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp -I. -Iinclude -Iimgui -Iimgui/backends libglfw3.a -lopengl32 -lgdi32 -o Crypto_Benchmark.exe -static-libstdc++
./Crypto_Benchmark.exe
```

### 4.3. Chạy kiểm định NIST Statistical Test Suite (Linux/WSL)
Để xác thực độ an toàn thống kê, sinh file dữ liệu `rubik_data.bin` và đẩy vào bộ test NIST:

**Sinh dữ liệu test (12.5 MB):**
```bash
gcc ./tests/gen_data.c ./src/rubik4d.c -Iinclude -o ./tests/gen_data.exe -O3 
>> ./tests/gen_data.exe
```

**Cài đặt & chạy NIST STS:**
```bash
sudo apt update && sudo apt install -y build-essential git libfftw3-dev
git clone https://github.com/arcetri/sts.git
cd sts 
make

# Chạy test với luồng dữ liệu của Rubik-4D
./sts -i 95 -w . -F r ../rubik_data.bin
```

## 🛠 5. Kế hoạch nghiên cứu & Nâng cấp (Roadmap)
- [x] **Xây dựng lõi C/C++ hoàn chỉnh:** Hiện thực hóa bảng hoán vị Rubik-4D tối ưu $O(1)$ và GUI Benchmark.
- [x] **Nâng cấp Key Schedule & Chế độ hoạt động:** Thay thế LCG bằng mạng SPN an toàn tuyệt đối và tích hợp chế độ CBC chuẩn mực.
- [x] **Tối ưu hóa vòng lặp (Round Reduction):** Phân tích và rút gọn thành công từ 12 vòng xuống 6 vòng, đẩy thông lượng phần mềm vượt mốc 75 MB/s.
- [x] **Kiểm định thống kê:** Vượt qua bộ tiêu chuẩn NIST SP 800-22 (186/188 sub-tests passed).
- [ ] **Chứng minh an toàn bằng MILP / SAT Solver:** Mô hình hóa toán học hệ phương trình vi sai/tuyến tính để xác định số hộp Active S-boxes tối thiểu.
- [ ] **Benchmark phần cứng (Hardware Synthesis):** Viết lại thuật toán bằng Verilog để tổng hợp lên FPGA, đo lường chính xác diện tích mạch (Gate Equivalents) và năng lượng tiêu thụ.

## 📄 6. Giấy phép & Tuyên bố học thuật
* Dự án được phân phối dưới giấy phép **MIT License**.
* Thiết kế tuân thủ nghiêm ngặt **Nguyên lý Kerckhoffs**: Độ an toàn của hệ thống hoàn toàn dựa vào tính bí mật của khóa, không phụ thuộc vào việc che giấu thuật toán.
