# 🧊 Rubik-4D Cipher
> A 128-bit Symmetric Lightweight Block Cipher based on 4D Tesseract Geometry & ARX Diffusion

---

## 📌 1. Overview

**Rubik-4D Cipher** is a lightweight symmetric block cipher designed for embedded systems, IoT microcontrollers, and application-specific hardware circuits.

### Problem Statement:
* Standard algorithms such as AES rely on finite field matrix multiplication over $GF(2^8)$ (`MixColumns`), incurring high hardware area overhead (Gate Equivalents) and power consumption.
* Pure ARX networks (such as Speck/Simon) are computationally lightweight, but their bit diffusion across branches is relatively slow, requiring a high round count.

### The Rubik-4D Approach:
* **4D Hypercube (Tesseract) Mapping:** 128 bits (16 bytes) map directly onto the 16 vertices of a 4-dimensional binary hypercube space $2 \times 2 \times 2 \times 2$.
* **$O(1)$ Permutation via the $SO(4)$ Rotation Group:** 12 orthogonal rotations are precomputed into lookup tables, achieving instantaneous byte transposition with zero computational overhead.
* **ARX Ripple-Carry Diffusion Layer:** Leverages modulo-256 addition carry propagation to rapidly diffuse differential changes across adjacent bytes.
* **Maximum Throughput with 6 Rounds:** Eliminates performance bottlenecks, boosting pure software throughput to ~79 MB/s—outperforming AES in software benchmarks while maintaining strong cryptographic security.

---

## 📐 2. Algorithm Architecture

The cipher processes a 128-bit block through **6 rounds** using the following pipeline:

```text
[Plaintext 128-bit] ──> ⊕ AddRoundKey(0)
                             │
        ┌─────────────────────┴────────────────────┐
        │               ROUND (1 - 6)              │
        │                                          │
        │  1. Non-linear S-Box (AES Substitution)  │
        │  2. 4D Tesseract Rotation (SO(4))        │
        │  3. Ripple-Carry Diffusion (ARX)         │
        │  4. Subkey Mixing (⊕ AddRoundKey)        │
        └─────────────────────┬────────────────────┘
                             │
                             ▼
                    [Ciphertext 128-bit]
```

**Core Security Features:**
* **SPN (Substitution-Permutation Network) Key Schedule:** A robust non-linear key expansion schedule that eliminates LCG-based weaknesses and resists related-key attacks.
* **CBC (Cipher Block Chaining) Mode:** Fully supports standard CBC mode with initialization vectors (IV) for secure bulk data processing.

---

## 📊 3. Specifications & Comparison

| Metric | AES-128 | Speck / Simon | Rubik-4D (Proposed) |
| :--- | :--- | :--- | :--- |
| **Block Size** | 128 bits | 64 / 128 bits | 128 bits |
| **Key Size** | 128 bits | 128 bits | 128 bits |
| **Rounds** | 10 | 32 | 6 |
| **Network Structure** | Pure SPN | Feistel / ARX | SPN combined with ARX & 4D Geometry |
| **Permutation Layer (P-box)** | Static ShiftRows | Cyclic Bit Shifts | Key-Dependent Dynamic 4D Tesseract Rotation |
| **Throughput (x86 SW)** | ~ 45 MB/s | Very High | ~ 79 MB/s |
| **NIST SP 800-22** | Passed | Passed | Passed (186/188 tests) |

---

## 🚀 4. Quick Start

### 4.1. Clone Repository
```bash
git clone [https://github.com/toilacobemongmo/Rubic4DCrypto.git](https://github.com/toilacobemongmo/Rubic4DCrypto.git)
cd Rubic4DCrypto
```

### 4.2. Build & Run GUI Benchmark (C++ / ImGui)
A real-time GUI application to benchmark Rubik-4D against AES and Speck:
```bash
g++ -O3 -std=c++17 src/main.cpp src/rubik4d.c src/aes128.c src/speck128.c src/analysis.cpp imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp -I. -Iinclude -Iimgui -Iimgui/backends libglfw3.a -lopengl32 -lgdi32 -o Crypto_Benchmark.exe -static-libstdc++
./Crypto_Benchmark.exe
```

### 4.3. Run NIST Statistical Test Suite (Linux/WSL)
To verify statistical randomness, generate the `rubik_data.bin` stream and run the NIST test suite:

**Generate test stream (12.5 MB):**
```bash
gcc ./tests/gen_data.c ./src/rubik4d.c -Iinclude -o ./tests/gen_data.exe -O3 
./tests/gen_data.exe
```

**Install & execute NIST STS:**
```bash
sudo apt update && sudo apt install -y build-essential git libfftw3-dev
git clone [https://github.com/arcetri/sts.git](https://github.com/arcetri/sts.git)
cd sts 
make

# Run the test against the Rubik-4D binary output
./sts -i 95 -w . -F r ../rubik_data.bin
```

---

## 🛠 5. Research Roadmap

- [x] **Complete C/C++ Core Engine:** Implement the $O(1)$ Rubik-4D permutation lookup tables and GUI Benchmark.
- [x] **Key Schedule & Cipher Mode Hardening:** Replace LCG with a cryptographically sound SPN key schedule and integrate standard CBC mode.
- [x] **Round Optimization:** Analyze differential bounds and reduce rounds from 12 to 6, achieving software throughput in excess of 75 MB/s.
- [x] **Statistical Validation:** Pass the NIST SP 800-22 test suite (186/188 sub-tests passed).
- [ ] **MILP / SAT-Based Cryptanalysis:** Construct mathematical programming models for differential and linear characteristics to prove lower bounds on active S-boxes.
- [ ] **Hardware Synthesis & Benchmarking:** Implement the cipher in synthesizable Verilog for FPGA/ASIC targets to evaluate exact Gate Equivalents (GE) and power efficiency.

---

## 📄 6. License & Academic Disclaimer

* Distributed under the **MIT License**.
* The design strictly adheres to **Kerckhoffs's Principle**: cryptographic security rests entirely on key confidentiality, not on the secrecy of the algorithm.
* Project Preprint / Paper: **https://www.overleaf.com/read/tdbnmfjskpmj#8dd4cd**
