<div align="center">

# 🧊 Rubik-4D Cipher
### 128-bit Symmetric Lightweight Block Cipher via 4D Tesseract Geometry & ARX

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Algorithm](https://img.shields.io/badge/Block_Size-128_bit-blue.svg)](#)
[![Design](https://img.shields.io/badge/Architecture-SPN_%2B_ARX-green.svg)](#)
[![Status](https://img.shields.io/badge/Status-Research_Draft-orange.svg)](#)

<p align="center">
  <b>Thuật toán mã hóa khối hạng nhẹ kết hợp giữa mô hình siêu lập phương 4 chiều (Tesseract) và tầng khuếch tán ARX cho thiết bị nhúng / IoT.</b>
</p>

---
</div>

## 📌 Điểm nhấn kỹ thuật

> **Vấn đề cốt lõi:** Các cipher cổ điển như AES dùng phép nhân ma trận $GF(2^8)$ (`MixColumns`) rất tốn tài nguyên phần cứng. 
> 
> **Giải pháp Rubik-4D:** Thay thế hoàn toàn phép nhân trường bằng **phép quay trực giao trong không gian 4 chiều** kết hợp **cộng dồn bit nhớ ARX**, cho phép đạt độ khuếch tán cực nhanh với chi phí phần cứng $O(1)$.

<br>

## 📐 Kiến trúc & Thiết kế toán học

Thuật toán thực thi tuần tự **12 vòng lặp (rounds)** trên khối dữ liệu 128-bit:

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
