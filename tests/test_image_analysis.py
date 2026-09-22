import os
import sys
import json
import ctypes
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image
from scipy.stats import chi2

# Load Rubik-4D shared library
dll_path = os.path.join(os.path.dirname(__file__), "bin", "librubik4d.dll")
if not os.path.exists(dll_path):
    dll_path = os.path.join(os.path.dirname(__file__), "librubik4d.dll")
if not os.path.exists(dll_path):
    raise FileNotFoundError(f"Cannot find librubik4d.dll in tests/bin or tests/. Compile it first.")

lib = ctypes.CDLL(dll_path)
lib.rubik4d_init_tables.restype = None
lib.rubik4d_init_tables.argtypes = []
lib.rubik4d_init_tables()

lib.rubik4d_encrypt.restype = ctypes.c_size_t
lib.rubik4d_encrypt.argtypes = [
    ctypes.c_char_p,
    ctypes.c_size_t,
    ctypes.c_char_p,
    ctypes.c_char_p,
    ctypes.c_size_t,
    ctypes.c_char_p
]

def encrypt_rubik4d_cbc(data_bytes, key_bytes, iv_bytes):
    """Encrypts byte buffer using Rubik-4D in CBC mode."""
    in_len = len(data_bytes)
    # PKCS#7 adds up to 16 bytes
    out_buf = ctypes.create_string_buffer(in_len + 32)
    total_len = lib.rubik4d_encrypt(
        data_bytes, in_len, out_buf, key_bytes, len(key_bytes), iv_bytes
    )
    # Return exactly the first in_len bytes for 1:1 image pixel mapping
    return out_buf.raw[:in_len]

def compute_entropy(img_arr):
    """Computes Shannon Information Entropy (Theoretical max for 8-bit is 8.0000)."""
    counts = np.bincount(img_arr.flatten(), minlength=256)
    probs = counts / float(img_arr.size)
    probs = probs[probs > 0]
    return float(-np.sum(probs * np.log2(probs)))

def compute_correlations(img_arr, num_samples=10000, seed=42):
    """Calculates Pearson correlation coefficients for Horizontal, Vertical, Diagonal."""
    h, w = img_arr.shape
    rng = np.random.default_rng(seed)
    x = rng.integers(0, h - 1, size=num_samples)
    y = rng.integers(0, w - 1, size=num_samples)

    # Horizontal
    h1 = img_arr[x, y].astype(np.float64)
    h2 = img_arr[x, y + 1].astype(np.float64)
    r_h = float(np.corrcoef(h1, h2)[0, 1])

    # Vertical
    v1 = img_arr[x, y].astype(np.float64)
    v2 = img_arr[x + 1, y].astype(np.float64)
    r_v = float(np.corrcoef(v1, v2)[0, 1])

    # Diagonal
    d1 = img_arr[x, y].astype(np.float64)
    d2 = img_arr[x + 1, y + 1].astype(np.float64)
    r_d = float(np.corrcoef(d1, d2)[0, 1])

    return r_h, r_v, r_d

def compute_chi_square(img_arr):
    """Computes Chi-Square goodness-of-fit test for uniform distribution."""
    observed = np.bincount(img_arr.flatten(), minlength=256)
    expected = img_arr.size / 256.0
    chi_val = float(np.sum((observed - expected) ** 2 / expected))
    p_val = float(1.0 - chi2.cdf(chi_val, df=255))
    return chi_val, p_val

def compute_npcr_uaci(c1_arr, c2_arr):
    """Computes NPCR (%) and UACI (%) between two encrypted images."""
    total = float(c1_arr.size)
    diff_mask = (c1_arr != c2_arr).astype(np.float64)
    npcr = float(np.sum(diff_mask) / total) * 100.0

    diff_intensity = np.abs(c1_arr.astype(np.float64) - c2_arr.astype(np.float64))
    uaci = float(np.sum(diff_intensity) / (255.0 * total)) * 100.0

    return npcr, uaci

def plot_and_save_figure(plain_arr, cipher_arr, img_name, output_path):
    """Renders side-by-side comparison images and grayscale histograms."""
    fig, axes = plt.subplots(2, 2, figsize=(11, 9))

    # Plain Image
    axes[0, 0].imshow(plain_arr, cmap="gray", vmin=0, vmax=255)
    axes[0, 0].set_title(f"Plaintext Image: {img_name}", fontsize=12, fontweight="bold")
    axes[0, 0].axis("off")

    # Plain Histogram
    axes[0, 1].hist(plain_arr.flatten(), bins=256, range=(0, 256), color="#1f4e79", alpha=0.85)
    axes[0, 1].set_title(f"Histogram: {img_name} (Original)", fontsize=12, fontweight="bold")
    axes[0, 1].set_xlabel("Grayscale Intensity Level (0 - 255)")
    axes[0, 1].set_ylabel("Pixel Frequency")
    axes[0, 1].grid(True, linestyle=":", alpha=0.6)

    # Cipher Image
    axes[1, 0].imshow(cipher_arr, cmap="gray", vmin=0, vmax=255)
    axes[1, 0].set_title(f"Ciphertext Image: {img_name} (Rubik-4D CBC)", fontsize=12, fontweight="bold")
    axes[1, 0].axis("off")

    # Cipher Histogram
    axes[1, 1].hist(cipher_arr.flatten(), bins=256, range=(0, 256), color="#c00000", alpha=0.85)
    axes[1, 1].axhline(y=1024, color="black", linestyle="--", linewidth=1.2, label="Ideal Uniform ($e=1024$)")
    axes[1, 1].set_title(f"Histogram: {img_name} (Encrypted - Uniform)", fontsize=12, fontweight="bold")
    axes[1, 1].set_xlabel("Grayscale Intensity Level (0 - 255)")
    axes[1, 1].set_ylabel("Pixel Frequency")
    axes[1, 1].grid(True, linestyle=":", alpha=0.6)
    axes[1, 1].legend()

    plt.tight_layout()
    plt.savefig(output_path, dpi=300)
    plt.close()
    print(f"[+] Saved high-resolution plot to {output_path}")

def analyze_image(img_path, img_name):
    print("=" * 80)
    print(f" ANALYZING BENCHMARK IMAGE: {img_name} ({img_path})")
    print("=" * 80)

    # Load image as 512x512 grayscale
    img = Image.open(img_path).convert("L")
    if img.size != (512, 512):
        img = img.resize((512, 512))
    plain_arr = np.array(img, dtype=np.uint8)
    plain_bytes = plain_arr.tobytes()

    # Standard secret key and random IV
    master_key = b"Rubik4DMasterKey"  # 16 bytes
    iv = b"\x01\x23\x45\x67\x89\xab\xcd\xef\xfe\xdc\xba\x98\x76\x54\x32\x10"  # 16 bytes

    # Encrypt primary image
    cipher_bytes = encrypt_rubik4d_cbc(plain_bytes, master_key, iv)
    cipher_arr = np.frombuffer(cipher_bytes, dtype=np.uint8).reshape((512, 512))

    # 1. Information Entropy
    h_plain = compute_entropy(plain_arr)
    h_cipher = compute_entropy(cipher_arr)

    # 2. Adjacent Pixel Correlation
    rh_p, rv_p, rd_p = compute_correlations(plain_arr, num_samples=10000)
    rh_c, rv_c, rd_c = compute_correlations(cipher_arr, num_samples=10000)

    # 3. Chi-Square Test
    chi_p, pval_p = compute_chi_square(plain_arr)
    chi_c, pval_c = compute_chi_square(cipher_arr)

    print(f"[*] Basic Cryptographic Metrics for {img_name}:")
    print(f"    - Shannon Entropy (Plain)     : {h_plain:.6f}")
    print(f"    - Shannon Entropy (Cipher)    : {h_cipher:.6f} (Ideal: 8.000000)")
    print(f"    - Plain Correlation (H / V / D): {rh_p:.6f} / {rv_p:.6f} / {rd_p:.6f}")
    print(f"    - Cipher Correlation (H)      : {rh_c:.6f} (Ideal: ~0.0000)")
    print(f"    - Cipher Correlation (V)      : {rv_c:.6f} (Ideal: ~0.0000)")
    print(f"    - Cipher Correlation (D)      : {rd_c:.6f} (Ideal: ~0.0000)")
    print(f"    - Plain Chi-Square (chi^2)    : {chi_p:,.2f}")
    print(f"    - Cipher Chi-Square (chi^2)   : {chi_c:.4f} (p-value: {pval_c:.6f}, Threshold alpha=0.01: 310.4574, Pass: {chi_c < 310.4574})")

    # 4. NPCR & UACI Evaluation
    # Mode A: Plaintext 1-bit Sensitivity (100 trials)
    # Lật 1 bit trong pixel đầu tiên (hoặc block đầu tiên) để đo khả năng lan truyền CBC toàn ảnh
    npcr_pt_list = []
    uaci_pt_list = []
    
    rng = np.random.default_rng(2026)
    num_trials = 100

    print(f"[*] Running {num_trials} trials of Plaintext 1-bit Difference (NPCR & UACI)...")
    for trial in range(num_trials):
        mod_arr = plain_arr.copy()
        # Flip 1 bit randomly within the first 16-byte block
        px_x = int(rng.integers(0, 16))
        bit_pos = int(rng.integers(0, 8))
        mod_arr[0, px_x] ^= (1 << bit_pos)

        mod_bytes = mod_arr.tobytes()
        cipher2_bytes = encrypt_rubik4d_cbc(mod_bytes, master_key, iv)
        cipher2_arr = np.frombuffer(cipher2_bytes, dtype=np.uint8).reshape((512, 512))

        npcr_val, uaci_val = compute_npcr_uaci(cipher_arr, cipher2_arr)
        npcr_pt_list.append(npcr_val)
        uaci_pt_list.append(uaci_val)

    mean_npcr_pt = float(np.mean(npcr_pt_list))
    std_npcr_pt = float(np.std(npcr_pt_list))
    mean_uaci_pt = float(np.mean(uaci_pt_list))
    std_uaci_pt = float(np.std(uaci_pt_list))

    print(f"    -> Plaintext NPCR (Mean +/- Std): {mean_npcr_pt:.4f}% +/- {std_npcr_pt:.4f}% (Ideal >= 99.6094%)")
    print(f"    -> Plaintext UACI (Mean +/- Std): {mean_uaci_pt:.4f}% +/- {std_uaci_pt:.4f}% (Ideal ~ 33.4635%)")

    # Mode B: Key 1-bit Sensitivity (100 trials)
    # Giữ nguyên ảnh gốc, lật 1 bit ngẫu nhiên trong 128-bit Master Key
    npcr_key_list = []
    uaci_key_list = []

    print(f"[*] Running {num_trials} trials of Key 1-bit Difference (NPCR & UACI)...")
    for trial in range(num_trials):
        key_byte_arr = bytearray(master_key)
        bit_idx = rng.integers(0, 128)
        key_byte_arr[bit_idx // 8] ^= (1 << (bit_idx % 8))
        mod_key = bytes(key_byte_arr)

        cipher_k2_bytes = encrypt_rubik4d_cbc(plain_bytes, mod_key, iv)
        cipher_k2_arr = np.frombuffer(cipher_k2_bytes, dtype=np.uint8).reshape((512, 512))

        npcr_val, uaci_val = compute_npcr_uaci(cipher_arr, cipher_k2_arr)
        npcr_key_list.append(npcr_val)
        uaci_key_list.append(uaci_val)

    mean_npcr_key = float(np.mean(npcr_key_list))
    std_npcr_key = float(np.std(npcr_key_list))
    mean_uaci_key = float(np.mean(uaci_key_list))
    std_uaci_key = float(np.std(uaci_key_list))

    print(f"    -> Key NPCR (Mean +/- Std)      : {mean_npcr_key:.4f}% +/- {std_npcr_key:.4f}% (Ideal >= 99.6094%)")
    print(f"    -> Key UACI (Mean +/- Std)      : {mean_uaci_key:.4f}% +/- {std_uaci_key:.4f}% (Ideal ~ 33.4635%)")

    # Save visualization figure directly to reports/
    reports_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "reports")
    os.makedirs(reports_dir, exist_ok=True)
    out_fig = os.path.join(reports_dir, f"{img_name}_cryptanalysis_eval.png")
    plot_and_save_figure(plain_arr, cipher_arr, img_name, out_fig)

    results = {
        "img_name": img_name,
        "h_plain": h_plain,
        "h_cipher": h_cipher,
        "rh_p": rh_p, "rv_p": rv_p, "rd_p": rd_p,
        "rh_c": rh_c, "rv_c": rv_c, "rd_c": rd_c,
        "chi_p": chi_p, "chi_c": chi_c, "pval_c": pval_c,
        "npcr_pt_mean": mean_npcr_pt, "npcr_pt_std": std_npcr_pt,
        "uaci_pt_mean": mean_uaci_pt, "uaci_pt_std": std_uaci_pt,
        "npcr_key_mean": mean_npcr_key, "npcr_key_std": std_npcr_key,
        "uaci_key_mean": mean_uaci_key, "uaci_key_std": std_uaci_key,
    }
    return results

def main():
    test_file_dir = os.path.join(os.path.dirname(__file__), "test_file")
    lena_path = os.path.join(test_file_dir, "Lena.png")
    baboon_path = os.path.join(test_file_dir, "Baboon.png")

    if not os.path.exists(lena_path):
        raise FileNotFoundError(f"Lena image not found at {lena_path}")
    if not os.path.exists(baboon_path):
        raise FileNotFoundError(f"Baboon image not found at {baboon_path}")

    res_lena = analyze_image(lena_path, "Lena")
    res_baboon = analyze_image(baboon_path, "Baboon")

    print("\n" + "=" * 80)
    print(" SUMMARY IMAGE CRYPTANALYSIS COMPARATIVE RESULTS")
    print("=" * 80)
    print(f"{'Metric':<36} | {'Lena (512x512)':<20} | {'Baboon (512x512)':<20} | {'Ideal Value'}")
    print("-" * 88)
    print(f"{'Entropy Plain':<36} | {res_lena['h_plain']:<20.6f} | {res_baboon['h_plain']:<20.6f} | (Content Dep)")
    print(f"{'Entropy Cipher (H)':<36} | {res_lena['h_cipher']:<20.6f} | {res_baboon['h_cipher']:<20.6f} | 8.000000")
    print(f"{'Corr Horizontal (Cipher)':<36} | {res_lena['rh_c']:<20.6f} | {res_baboon['rh_c']:<20.6f} | ~ 0.000000")
    print(f"{'Corr Vertical (Cipher)':<36} | {res_lena['rv_c']:<20.6f} | {res_baboon['rv_c']:<20.6f} | ~ 0.000000")
    print(f"{'Corr Diagonal (Cipher)':<36} | {res_lena['rd_c']:<20.6f} | {res_baboon['rd_c']:<20.6f} | ~ 0.000000")
    print(f"{'Chi-Square (chi^2)':<36} | {res_lena['chi_c']:<20.4f} | {res_baboon['chi_c']:<20.4f} | < 310.4574")
    print(f"{'NPCR Plaintext 1-bit':<36} | {res_lena['npcr_pt_mean']:<19.4f}% | {res_baboon['npcr_pt_mean']:<19.4f}% | >= 99.6094%")
    print(f"{'UACI Plaintext 1-bit':<36} | {res_lena['uaci_pt_mean']:<19.4f}% | {res_baboon['uaci_pt_mean']:<19.4f}% | ~ 33.4635%")
    print(f"{'NPCR Key 1-bit':<36} | {res_lena['npcr_key_mean']:<19.4f}% | {res_baboon['npcr_key_mean']:<19.4f}% | >= 99.6094%")
    print(f"{'UACI Key 1-bit':<36} | {res_lena['uaci_key_mean']:<19.4f}% | {res_baboon['uaci_key_mean']:<19.4f}% | ~ 33.4635%")
    print("=" * 88 + "\n")

    # Export to reports/image_analysis.json
    reports_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "reports")
    os.makedirs(reports_dir, exist_ok=True)
    json_path = os.path.join(reports_dir, "image_analysis.json")
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump({"Lena": res_lena, "Baboon": res_baboon}, f, indent=2)
    print(f"[+] Saved image cryptanalysis JSON to {json_path}")

if __name__ == "__main__":
    main()
