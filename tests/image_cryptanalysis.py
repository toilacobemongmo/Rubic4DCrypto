import sys
import os
import argparse
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image
import scipy.stats as stats


def load_image_or_bin(path, width=512, height=512):
    """Loads image data from standard formats (.png, .bmp) or raw binary files (.bin)."""
    if not os.path.exists(path):
        raise FileNotFoundError(f"File not found: {path}")

    total_pixels = width * height

    # Attempt to load using PIL first (standard image files)
    try:
        img = Image.open(path).convert("L")
        if img.size != (width, height):
            img = img.resize((width, height))
        return np.array(img, dtype=np.uint8)
    except Exception:
        # Fall back to raw binary parsing if PIL fails (e.g., raw ciphertext streams)
        with open(path, "rb") as f:
            raw_data = f.read()

        if len(raw_data) < total_pixels:
            raise ValueError(
                f"File {path} contains only {len(raw_data)} bytes; "
                f"expected at least {width}x{height} = {total_pixels} bytes."
            )

        # Slice the exact pixel buffer from the stream
        offset = len(raw_data) - total_pixels if len(raw_data) > total_pixels else 0
        arr = np.frombuffer(
            raw_data[offset : offset + total_pixels], dtype=np.uint8
        ).reshape((height, width))
        return arr


def compute_entropy(img_arr):
    """Computes Shannon Information Entropy (Theoretical maximum for 8-bit grayscale is 8.0000)."""
    counts = np.bincount(img_arr.flatten(), minlength=256)
    probs = counts / float(img_arr.size)
    probs = probs[probs > 0]
    return -np.sum(probs * np.log2(probs))


def compute_correlations(img_arr, num_samples=5000):
    """Calculates Pearson correlation coefficients between adjacent pixels (Horizontal, Vertical, Diagonal)."""
    h, w = img_arr.shape
    np.random.seed(42)
    x = np.random.randint(0, h - 1, num_samples)
    y = np.random.randint(0, w - 1, num_samples)

    # Horizontal
    h_pairs_x = img_arr[x, y]
    h_pairs_y = img_arr[x, y + 1]
    r_h = np.corrcoef(h_pairs_x, h_pairs_y)[0, 1]

    # Vertical
    v_pairs_x = img_arr[x, y]
    v_pairs_y = img_arr[x + 1, y]
    r_v = np.corrcoef(v_pairs_x, v_pairs_y)[0, 1]

    # Diagonal
    d_pairs_x = img_arr[x, y]
    d_pairs_y = img_arr[x + 1, y + 1]
    r_d = np.corrcoef(d_pairs_x, d_pairs_y)[0, 1]

    return r_h, r_v, r_d


def compute_chi_square(img_arr):
    """Computes Chi-Square goodness-of-fit test for uniform pixel distribution (Critical value at alpha=0.05 is 293.2478)."""
    observed = np.bincount(img_arr.flatten(), minlength=256)
    expected = img_arr.size / 256.0
    chi2 = np.sum((observed - expected) ** 2 / expected)
    return chi2


def compute_npcr_uaci(c1, c2):
    """Computes differential sensitivity metrics: NPCR (>= 99.6094%) and UACI (~ 33.4635%)."""
    h, w = c1.shape
    total = float(h * w)

    diff_mask = (c1 != c2).astype(np.float64)
    npcr = (np.sum(diff_mask) / total) * 100.0

    diff_intensity = np.abs(c1.astype(np.float64) - c2.astype(np.float64))
    uaci = (np.sum(diff_intensity) / (255.0 * total)) * 100.0

    return npcr, uaci


def plot_analysis(cipher_arr, plain_arr=None, output_fig="image_analysis_result.png"):
    """Renders side-by-side comparison images and grayscale histograms."""
    if plain_arr is not None:
        fig, axes = plt.subplots(2, 2, figsize=(11, 9))

        # Plain Image
        axes[0, 0].imshow(plain_arr, cmap="gray", vmin=0, vmax=255)
        axes[0, 0].set_title("Plaintext Image (Original)", fontsize=12)
        axes[0, 0].axis("off")

        # Plain Histogram
        axes[0, 1].hist(
            plain_arr.flatten(), bins=256, range=(0, 256), color="navy", alpha=0.75
        )
        axes[0, 1].set_title("Plaintext Image Histogram", fontsize=12)
        axes[0, 1].set_xlabel("Grayscale Intensity Level")
        axes[0, 1].set_ylabel("Frequency Distribution")
        axes[0, 1].grid(True, linestyle=":", alpha=0.6)

        # Cipher Image
        axes[1, 0].imshow(cipher_arr, cmap="gray", vmin=0, vmax=255)
        axes[1, 0].set_title("Ciphertext Image (Rubik-4D)", fontsize=12)
        axes[1, 0].axis("off")

        # Cipher Histogram
        axes[1, 1].hist(
            cipher_arr.flatten(), bins=256, range=(0, 256), color="crimson", alpha=0.75
        )
        axes[1, 1].set_title("Ciphertext Histogram (Uniform Distribution)", fontsize=12)
        axes[1, 1].set_xlabel("Grayscale Intensity Level")
        axes[1, 1].set_ylabel("Frequency Distribution")
        axes[1, 1].grid(True, linestyle=":", alpha=0.6)
    else:
        fig, axes = plt.subplots(1, 2, figsize=(11, 4.5))

        axes[0].imshow(cipher_arr, cmap="gray", vmin=0, vmax=255)
        axes[0].set_title("Ciphertext Image (Rubik-4D)", fontsize=12)
        axes[0].axis("off")

        axes[1].hist(
            cipher_arr.flatten(), bins=256, range=(0, 256), color="crimson", alpha=0.75
        )
        axes[1].set_title("Ciphertext Histogram (Uniform Distribution)", fontsize=12)
        axes[1].set_xlabel("Grayscale Intensity Level")
        axes[1].set_ylabel("Frequency Distribution")
        axes[1].grid(True, linestyle=":", alpha=0.6)

    plt.tight_layout()
    plt.savefig(output_fig, dpi=300)
    print(f"[+] High-resolution analysis figure saved to: {output_fig}")


def main():
    parser = argparse.ArgumentParser(
        description="Image Cryptanalysis and Statistical Metric Evaluation for Rubik-4D Block Cipher."
    )
    parser.add_argument("cipher_file", help="Path to primary ciphertext file (.bin or image format)")
    parser.add_argument(
        "--plain",
        "-p",
        help="Path to original plaintext image (for comparative assessment)",
        default=None,
    )
    parser.add_argument(
        "--cipher2",
        help="Path to secondary ciphertext file with 1-bit difference (for NPCR/UACI computation)",
        default=None,
    )
    parser.add_argument("--width", "-W", type=int, default=512, help="Image width in pixels (default: 512)")
    parser.add_argument("--height", "-H", type=int, default=512, help="Image height in pixels (default: 512)")
    parser.add_argument(
        "--save-dec-img",
        help="Export parsed byte matrix to a PNG visual image",
        default=None,
    )

    args = parser.parse_args()

    print(f"[*] Loading ciphertext: {args.cipher_file}...")
    cipher_arr = load_image_or_bin(args.cipher_file, args.width, args.height)

    if args.save_dec_img:
        out_img = Image.fromarray(cipher_arr)
        out_img.save(args.save_dec_img)
        print(f"[+] Binary data exported as visual image: {args.save_dec_img}")

    plain_arr = None
    if args.plain:
        plain_arr = load_image_or_bin(args.plain, args.width, args.height)

    # Compute cipher statistical metrics
    entropy_c = compute_entropy(cipher_arr)
    rh_c, rv_c, rd_c = compute_correlations(cipher_arr)
    chi2_c = compute_chi_square(cipher_arr)

    print("\n" + "=" * 76)
    print(f"{'Security Evaluation Metric':<40} | {'Achieved Value':<15} | {'Theoretical Ideal'}")
    print("=" * 76)
    print(f"{'Information Entropy':<40} | {entropy_c:<15.6f} | ~ 8.0000")
    print(f"{'Adjacent Correlation - Horizontal (H)':<40} | {rh_c:<15.6f} | ~ 0.0000")
    print(f"{'Adjacent Correlation - Vertical (V)':<40} | {rv_c:<15.6f} | ~ 0.0000")
    print(f"{'Adjacent Correlation - Diagonal (D)':<40} | {rd_c:<15.6f} | ~ 0.0000")
    print(f"{'Chi-Square Goodness-of-Fit (chi^2)':<40} | {chi2_c:<15.2f} | < 293.2478 (alpha=0.05)")

    if plain_arr is not None:
        rh_p, rv_p, rd_p = compute_correlations(plain_arr)
        entropy_p = compute_entropy(plain_arr)
        print("-" * 76)
        print(f"{'Plaintext Information Entropy':<40} | {entropy_p:<15.6f} | (Content Dependent)")
        print(f"{'Plaintext Adjacent Correlation (H/V/D)':<40} | {rh_p:.3f} / {rv_p:.3f} / {rd_p:.3f} | ~ 0.90 - 0.99")

        if args.cipher2:
            cipher_arr2 = load_image_or_bin(args.cipher2, args.width, args.height)
            npcr, uaci = compute_npcr_uaci(cipher_arr, cipher_arr2)
            print("-" * 76)
            print(f"{'NPCR (Number of Pixels Change Rate)':<40} | {npcr:<15.4f}% | >= 99.6094%")
            print(f"{'UACI (Unified Average Changing Intensity)':<40} | {uaci:<15.4f}% | ~ 33.4635%")

    print("=" * 76 + "\n")

    plot_analysis(cipher_arr, plain_arr, output_fig="image_cryptanalysis_result.png")


if __name__ == "__main__":
    main()