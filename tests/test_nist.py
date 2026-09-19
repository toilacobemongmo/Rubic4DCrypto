import sys
import os
import math
import numpy as np
import scipy.special as sp

def bytes_to_bits(data):
    arr = np.frombuffer(data, dtype=np.uint8)
    return np.unpackbits(arr)

# 1. Frequency (Monobit) Test (NIST 2.1)
def test_monobit(bits):
    n = len(bits)
    sn = np.sum(2 * bits - 1)
    s_obs = abs(sn) / np.sqrt(n)
    p_val = sp.erfc(s_obs / np.sqrt(2))
    return "Frequency (Monobit)", p_val, p_val >= 0.01

# 2. Frequency Test within a Block (NIST 2.2)
def test_block_frequency(bits, m=128):
    n = len(bits)
    N = n // m
    if N == 0: return "Block Frequency", 0.0, False
    blocks = bits[:N*m].reshape(N, m)
    pi = np.mean(blocks, axis=1)
    chi2 = 4.0 * m * np.sum((pi - 0.5)**2)
    p_val = sp.gammaincc(N / 2.0, chi2 / 2.0)
    return f"Block Frequency (m={m})", p_val, p_val >= 0.01

# 3. Runs Test (NIST 2.3)
def test_runs(bits):
    n = len(bits)
    pi = np.mean(bits)
    if abs(pi - 0.5) >= (2.0 / np.sqrt(n)):
        return "Runs", 0.0, False
    v_obs = np.sum(bits[:-1] != bits[1:]) + 1
    num = abs(v_obs - 2.0 * n * pi * (1.0 - pi))
    den = 2.0 * np.sqrt(2.0 * n) * pi * (1.0 - pi)
    p_val = sp.erfc(num / den)
    return "Runs", p_val, p_val >= 0.01

# 4. Longest Run of Ones in a Block (NIST 2.4)
def test_longest_run(bits):
    n = len(bits)
    if n < 128: return "Longest Run of Ones", 0.0, False
    if n < 6272:
        m, k = 8, 3
        v_bounds = [1, 2, 3, 4]
        pi_vals = [0.2148, 0.3672, 0.2305, 0.1875]
    elif n < 750000:
        m, k = 128, 5
        v_bounds = [4, 5, 6, 7, 8, 9]
        pi_vals = [0.1174, 0.2430, 0.2493, 0.1752, 0.1027, 0.1124]
    else:
        m, k = 10000, 6
        v_bounds = [10, 11, 12, 13, 14, 15, 16]
        pi_vals = [0.0882, 0.2092, 0.2483, 0.1933, 0.1208, 0.0675, 0.0727]

    N = n // m
    blocks = bits[:N*m].reshape(N, m)
    counts = np.zeros(len(pi_vals))

    for b in blocks:
        # Tinh run 1 dai nhat
        max_run = cur_run = 0
        for bit in b:
            if bit == 1:
                cur_run += 1
                if cur_run > max_run: max_run = cur_run
            else:
                cur_run = 0
        # Phan loai vao bin
        if max_run <= v_bounds[0]:
            counts[0] += 1
        elif max_run >= v_bounds[-1]:
            counts[-1] += 1
        else:
            idx = max_run - v_bounds[0]
            counts[idx] += 1

    chi2 = np.sum((counts - N * np.array(pi_vals))**2 / (N * np.array(pi_vals)))
    p_val = sp.gammaincc(k / 2.0, chi2 / 2.0)
    return "Longest Run of Ones", p_val, p_val >= 0.01

# 5. Discrete Fourier Transform (Spectral) Test (NIST 2.6)
def test_spectral(bits):
    n = len(bits)
    x = 2 * bits - 1
    s = np.fft.fft(x)
    m = np.abs(s[:n // 2])
    t = np.sqrt(np.log(1.0 / 0.05) * n)
    n0 = 0.95 * (n / 2.0)
    n1 = np.sum(m < t)
    d = (n1 - n0) / np.sqrt(n * 0.95 * 0.05 / 4.0)
    p_val = sp.erfc(abs(d) / np.sqrt(2))
    return "Discrete Fourier Transform", p_val, p_val >= 0.01

# 6. Non-overlapping Template Matching (NIST 2.7)
def test_non_overlapping_template(bits, template="000000001", m=1024):
    n = len(bits)
    N = n // m
    if N == 0: return "Non-overlapping Template", 0.0, False
    b_len = len(template)
    target = np.array([int(c) for c in template], dtype=np.uint8)
    
    blocks = bits[:N*m].reshape(N, m)
    w = np.zeros(N)
    for i, blk in enumerate(blocks):
        match_count = 0
        j = 0
        while j <= m - b_len:
            if np.array_equal(blk[j:j+b_len], target):
                match_count += 1
                j += b_len
            else:
                j += 1
        w[i] = match_count

    mu = (m - b_len + 1) / (2.0 ** b_len)
    var = m * ((1.0 / (2.0 ** b_len)) - ((2.0 * b_len - 1.0) / (2.0 ** (2.0 * b_len))))
    chi2 = np.sum((w - mu)**2 / var)
    p_val = sp.gammaincc(N / 2.0, chi2 / 2.0)
    return "Non-overlapping Template (m=9)", p_val, p_val >= 0.01

# 7. Approximate Entropy Test (NIST 2.12)
def test_approximate_entropy(bits, m=8):
    n = len(bits)
    def phi(m_len):
        blocks = np.array([bits[(i + np.arange(m_len)) % n] for i in range(n)])
        weights = 1 << np.arange(m_len)[::-1]
        vals = np.dot(blocks, weights)
        counts = np.bincount(vals, minlength=1<<m_len)
        c_i = counts / float(n)
        c_i = c_i[c_i > 0]
        return np.sum(c_i * np.log(c_i))
    
    chi2 = 2.0 * n * (math.log(2) - (phi(m) - phi(m + 1)))
    p_val = sp.gammaincc(2**(m-1), chi2 / 2.0)
    return f"Approximate Entropy (m={m})", p_val, p_val >= 0.01

# 8. Cumulative Sums (Cusum) Test (NIST 2.13)
def test_cumulative_sums(bits):
    n = float(len(bits))
    x = 2.0 * bits.astype(np.float64) - 1.0
    s = np.cumsum(x)
    z = float(np.max(np.abs(s)))
    
    if z == 0.0:
        return "Cumulative Sums (Forward)", 1.0, True

    sqrt_n = np.sqrt(n)

    # Tính term 1
    k_start1 = int(math.floor((-n / z + 1.0) / 4.0))
    k_end1 = int(math.floor((n / z - 1.0) / 4.0))
    term1 = 0.0
    if k_start1 <= k_end1:
        k_arr1 = np.arange(k_start1, k_end1 + 1, dtype=np.float64)
        term1 = np.sum(sp.ndtr((4.0 * k_arr1 + 1.0) * z / sqrt_n) - sp.ndtr((4.0 * k_arr1 - 1.0) * z / sqrt_n))

    # Tính term 2
    k_start2 = int(math.floor((-n / z - 3.0) / 4.0))
    k_end2 = int(math.floor((n / z - 1.0) / 4.0))
    term2 = 0.0
    if k_start2 <= k_end2:
        k_arr2 = np.arange(k_start2, k_end2 + 1, dtype=np.float64)
        term2 = np.sum(sp.ndtr((4.0 * k_arr2 + 3.0) * z / sqrt_n) - sp.ndtr((4.0 * k_arr2 + 1.0) * z / sqrt_n))

    p_val = 1.0 - float(term1) + float(term2)
    p_val = max(0.0, min(1.0, p_val))
    return "Cumulative Sums (Forward)", p_val, p_val >= 0.01

def run_nist_suite(filepath):
    print(f"[*] Reading file: {filepath}")
    if not os.path.exists(filepath):
        print(f"[!] Error: File '{filepath}' not found!")
        return

    with open(filepath, "rb") as f:
        data = f.read()

    total_bits = len(data) * 8
    print(f"[*] Loaded {len(data):,} bytes ({total_bits:,} bits)")
    bits = bytes_to_bits(data)

    tests = [
        test_monobit(bits),
        test_block_frequency(bits, m=128),
        test_runs(bits),
        test_longest_run(bits),
        test_spectral(bits),
        test_non_overlapping_template(bits),
        test_approximate_entropy(bits, m=8),
        test_cumulative_sums(bits)
    ]

    print("\n" + "="*68)
    print(f"{'NIST SP 800-22 Test Item':<35} | {'P-Value':<12} | {'Verdict'}")
    print("="*68)
    passed_cnt = 0
    for name, p_val, verdict in tests:
        v_str = "PASSED" if verdict else "FAILED"
        if verdict: passed_cnt += 1
        print(f"{name:<35} | {p_val:<12.6f} | {v_str}")
    print("="*68)
    print(f"Summary: {passed_cnt}/{len(tests)} tests passed (alpha = 0.01)\n")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        run_nist_suite(sys.argv[1])
    else:
        # Đường dẫn mặc định file benchmark của bạn
        default_file = r"C:\Users\ghaob\Rubic4DCrypto\benchmark_data\test_1MB.bin"
        run_nist_suite(default_file)