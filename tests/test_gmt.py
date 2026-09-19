import sys
import numpy as np
import scipy.special as sp

def bytes_to_bits(data):
    # Chuyển đổi chuỗi byte sang mảng bit nhị phân {0, 1}
    arr = np.frombuffer(data, dtype=np.uint8)
    return np.unpackbits(arr)

# 1. Monobit Frequency Test (GM/T 0005 - 5.1)
def test_monobit(bits):
    n = len(bits)
    sn = np.sum(2 * bits - 1)
    s_obs = abs(sn) / np.sqrt(n)
    p_val = sp.erfc(s_obs / np.sqrt(2))
    return "Monobit Frequency", p_val, p_val >= 0.01

# 2. Block Frequency Test (m = 10000) (GM/T 0005 - 5.2)
def test_block_frequency(bits, m=10000):
    n = len(bits)
    N = n // m
    if N == 0: return "Block Frequency", 0.0, False
    blocks = bits[:N*m].reshape(N, m)
    pi = np.mean(blocks, axis=1)
    chi2 = 4.0 * m * np.sum((pi - 0.5)**2)
    p_val = sp.gammaincc(N / 2.0, chi2 / 2.0)
    return "Block Frequency", p_val, p_val >= 0.01

# 3. Poker Test (m = 4, 8) (GM/T 0005 - 5.3)
def test_poker(bits, m=4):
    n = len(bits)
    k = n // m
    if k == 0: return f"Poker (m={m})", 0.0, False
    blocks = bits[:k*m].reshape(k, m)
    # Tính giá trị nguyên của từng block m-bit
    weights = 1 << np.arange(m)[::-1]
    vals = np.dot(blocks, weights)
    counts = np.bincount(vals, minlength=1<<m)
    chi2 = ( (1 << m) / k ) * np.sum(counts**2) - k
    p_val = sp.gammaincc(( (1 << m) - 1 ) / 2.0, chi2 / 2.0)
    return f"Poker (m={m})", p_val, p_val >= 0.01

# 4. Runs Test (GM/T 0005 - 5.5)
def test_runs(bits):
    n = len(bits)
    pi = np.mean(bits)
    if abs(pi - 0.5) >= (2.0 / np.sqrt(n)):
        return "Runs Test", 0.0, False
    v_obs = np.sum(bits[:-1] != bits[1:]) + 1
    num = abs(v_obs - 2.0 * n * pi * (1.0 - pi))
    den = 2.0 * np.sqrt(2.0 * n) * pi * (1.0 - pi)
    p_val = sp.erfc(num / den)
    return "Runs Test", p_val, p_val >= 0.01

# 5. Binary Derivative Test (k = 3, 7) (GM/T 0005 - 5.8)
def test_binary_derivative(bits, k=3):
    cur = bits.copy()
    for _ in range(k):
        cur = np.bitwise_xor(cur[:-1], cur[1:])
    n_k = len(cur)
    sn = np.sum(2 * cur - 1)
    s_obs = abs(sn) / np.sqrt(n_k)
    p_val = sp.erfc(s_obs / np.sqrt(2))
    return f"Binary Derivative (k={k})", p_val, p_val >= 0.01

# 6. Autocorrelation Test (d = 1, 8, 16) (GM/T 0005 - 5.9)
def test_autocorrelation(bits, d=1):
    n = len(bits)
    a = np.bitwise_xor(bits[:n-d], bits[d:])
    v_obs = 2.0 * (np.sum(a) - ((n - d) / 2.0)) / np.sqrt(n - d)
    p_val = sp.erfc(abs(v_obs) / np.sqrt(2))
    return f"Autocorrelation (d={d})", p_val, p_val >= 0.01

# 7. Cumulative Sums Test (GM/T 0005 - 5.11)
def test_cumulative_sums(bits):
    n = len(bits)
    x = 2 * bits - 1
    s = np.cumsum(x)
    z = np.max(np.abs(s))
    
    # Xấp xỉ p-value theo GM/T & NIST
    k_range = np.arange(int((-n/z + 1)/4), int((n/z - 1)/4) + 1)
    term1 = sp.ndtr((4*k_range + 1)*z / np.sqrt(n)) - sp.ndtr((4*k_range - 1)*z / np.sqrt(n))
    k_range2 = np.arange(int((-n/z - 3)/4), int((n/z - 1)/4) + 1)
    term2 = sp.ndtr((4*k_range2 + 3)*z / np.sqrt(n)) - sp.ndtr((4*k_range2 + 1)*z / np.sqrt(n))
    p_val = 1.0 - np.sum(term1) + np.sum(term2)
    p_val = float(np.clip(p_val, 0.0, 1.0))
    return "Cumulative Sums", p_val, p_val >= 0.01

def run_all_gmt_tests(file_path):
    print(f"[*] Reading file: {file_path}")
    with open(file_path, "rb") as f:
        data = f.read()
    
    print(f"[*] Total bytes: {len(data):,} bytes ({len(data)*8:,} bits)")
    bits = bytes_to_bits(data)
    
    tests = [
        test_monobit(bits),
        test_block_frequency(bits, m=10000),
        test_poker(bits, m=4),
        test_poker(bits, m=8),
        test_runs(bits),
        test_binary_derivative(bits, k=3),
        test_binary_derivative(bits, k=7),
        test_autocorrelation(bits, d=1),
        test_autocorrelation(bits, d=2),
        test_autocorrelation(bits, d=8),
        test_autocorrelation(bits, d=16),
        test_cumulative_sums(bits)
    ]
    
    print("\n" + "="*65)
    print(f"{'GM/T 0005-2021 Test Item':<32} | {'P-Value':<12} | {'Verdict'}")
    print("="*65)
    passed_count = 0
    for name, p_val, passed in tests:
        verdict = "PASSED" if passed else "FAILED"
        if passed: passed_count += 1
        print(f"{name:<32} | {p_val:<12.6f} | {verdict}")
    print("="*65)
    print(f"Summary: {passed_count}/{len(tests)} tests passed (alpha = 0.01)\n")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python test_gmt.py <path_to_ciphertext.bin>")
    else:
        run_all_gmt_tests(sys.argv[1])