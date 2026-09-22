# 📁 Rubik-4D Cryptanalysis & Performance Reports

Thư mục này chứa toàn bộ báo cáo, bảng biểu LaTeX chuẩn hóa và dữ liệu thực nghiệm kiểm định mật mã của thuật toán **Rubik-4D** (thực thi trên hệ thống vi xử lý **13th Gen Intel Core i3-13100F @ 3.40GHz, GCC 16.2.0 -O3**).

---

## 📑 Danh mục các tài liệu và số liệu trong thư mục:

1. **Báo cáo chính:**
   - [`Rubik4D_Cryptanalysis_Report.md`](./Rubik4D_Cryptanalysis_Report.md): Báo cáo tổng hợp chi tiết bằng Markdown phân tích toàn diện 4 hạng mục kiểm thử (Key Schedule SAC & Weak Keys, Plaintext/Key Sensitivity & Avalanche Effect, Image Cryptanalysis, Software Throughput & Cycles/Byte).
   - [`Rubik4D_Tables.tex`](./Rubik4D_Tables.tex): File chứa toàn bộ 6 bảng biểu bằng mã nguồn LaTeX chuẩn (`booktabs`, `multirow`, `caption`, `label`) sẵn sàng sao chép và dán trực tiếp vào bản thảo bài báo khoa học (`haogianguyen.tex`).

2. **Đồ thị phân tích mật mã ảnh (300 DPI):**
   - [`Lena_cryptanalysis_eval.png`](./Lena_cryptanalysis_eval.png): Đồ thị so sánh ảnh gốc, ảnh mã hóa và phân bố lược đồ xám Histogram trên ảnh chuẩn Lena $512 \times 512$.
   - [`Baboon_cryptanalysis_eval.png`](./Baboon_cryptanalysis_eval.png): Đồ thị so sánh ảnh gốc, ảnh mã hóa và phân bố lược đồ xám Histogram trên ảnh chuẩn Baboon $512 \times 512$.

3. **Nhật ký dữ liệu đo đạc thô (Raw Benchmark & Test Outputs):**
   - [`raw_key_schedule_sac.txt`](./raw_key_schedule_sac.txt): Dữ liệu đo SAC trên 10.000 cặp khóa ngẫu nhiên và phân tích 9 mẫu khóa yếu cực đoan.
   - [`raw_sensitivity.txt`](./raw_sensitivity.txt): Dữ liệu đo hiệu ứng thác lũ bản rõ (Plaintext Avalanche) theo từng vòng $r = 1 \dots 8$ và độ nhạy khóa trên 10.000 mẫu.
   - [`raw_image_cryptanalysis.txt`](./raw_image_cryptanalysis.txt): Dữ liệu chi tiết Entropy, tương quan pixel lân cận 3 hướng, Chi-square test, NPCR & UACI (100 lần thử cho cả 1-bit bản rõ và 1-bit khóa).
   - [`raw_software_benchmark.txt`](./raw_software_benchmark.txt): Dữ liệu đo thời gian thực thi, thông lượng (MB/s), chu kỳ máy trên mỗi byte (cpb qua `__rdtsc`) từ 100 KB đến 20 MB so sánh giữa Rubik-4D, AES-128, Speck-128, Simon-128, ChaCha20.
