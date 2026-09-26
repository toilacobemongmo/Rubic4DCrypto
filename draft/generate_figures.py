import matplotlib.pyplot as plt
import matplotlib.patches as patches
from matplotlib.patches import FancyBboxPatch, ArrowStyle, Circle, Rectangle
from matplotlib.lines import Line2D
import numpy as np
import os

# Set global style for academic figures (IEEE standard fonts and clean look)
plt.rcParams['font.family'] = 'serif'
plt.rcParams['font.size'] = 10
plt.rcParams['axes.labelsize'] = 11
plt.rcParams['axes.titlesize'] = 12
plt.rcParams['xtick.labelsize'] = 10
plt.rcParams['ytick.labelsize'] = 10
plt.rcParams['legend.fontsize'] = 10
plt.rcParams['figure.titlesize'] = 13

OUTPUT_DIR = os.path.dirname(os.path.abspath(__file__))

# ==============================================================================
# FIGURE 1: 4D Tesseract Geometric Projection & State Byte Mapping
# ==============================================================================
def generate_fig_tesseract():
    fig, ax = plt.subplots(figsize=(7.5, 6.6), dpi=300)
    ax.set_aspect('equal')
    ax.axis('off')

    # Non-degenerate oblique perspective parameters
    # Resolves all vertex overlaps (including B6 vs B15 and B1 vs B8)
    s_out = 1.70
    s_in = 0.82
    kx = 0.60
    ky = 0.22

    # 8 vertices for a cube: (x, y, z) in {-1, 1}^3 projected to 2D
    # x: -1=left, +1=right
    # y: -1=bottom, +1=top
    # z: -1=front, +1=back (shifted by +kx*z, +ky*z)
    def project_cube(s):
        coords = []
        for x in [-1, 1]:
            for y in [-1, 1]:
                for z in [-1, 1]:
                    X = (x + kx * z) * s
                    Y = (y + ky * z) * s
                    coords.append((X, Y))
        return coords

    outer = project_cube(s_out)
    inner = project_cube(s_in)

    # 12 edges per cube
    cube_edges = [
        (0, 1), (2, 3), (4, 5), (6, 7), # depth (z-axis)
        (0, 2), (1, 3), (4, 6), (5, 7), # vertical (y-axis)
        (0, 4), (1, 5), (2, 6), (3, 7)  # horizontal (x-axis)
    ]

    # 1. Draw 8 connecting 4D hyper-edges (w-axis) - SOLID PURPLE LINES (ls='-')
    # Connects outer cell (w=0) to inner cell (w=1)
    for i in range(8):
        ax.plot([outer[i][0], inner[i][0]], [outer[i][1], inner[i][1]],
                color='#8E24AA', lw=2.2, ls='-', alpha=0.9, zorder=2)

    # 2. Draw outer cube edges (Dark Navy Blue)
    for u, v in cube_edges:
        ax.plot([outer[u][0], outer[v][0]], [outer[u][1], outer[v][1]],
                color='#1B365D', lw=2.4, zorder=3)

    # 3. Draw inner cube edges (Teal)
    for u, v in cube_edges:
        ax.plot([inner[u][0], inner[v][0]], [inner[u][1], inner[v][1]],
                color='#00897B', lw=2.0, zorder=3)

    # 4. Draw vertices with high-contrast colored nodes and bold white text
    for i in range(8):
        # Outer cube vertices B0..B7 (w = 0)
        px, py = outer[i]
        ax.scatter(px, py, s=320, color='#1B365D', edgecolors='#FFFFFF', lw=1.8, zorder=5)
        ax.text(px, py, f"$B_{{{i}}}$", color='white', ha='center', va='center',
                fontsize=9.5, fontweight='bold', zorder=6)

        # Inner cube vertices B8..B15 (w = 1)
        qx, qy = inner[i]
        ax.scatter(qx, qy, s=270, color='#C62828', edgecolors='#FFFFFF', lw=1.8, zorder=5)
        ax.text(qx, qy, f"$B_{{{i+8}}}$", color='white', ha='center', va='center',
                fontsize=9.0, fontweight='bold', zorder=6)

    # 5. Double rotation planes annotations (Clean placement outside vertices)
    # Plane 1: (X0, X1) on front face
    front_center = (-kx * s_out, -ky * s_out)
    ax.annotate(r"$\mathcal{P}_1: (X_0, X_1)$ Plane" + "\n" + r"Front 2D Rotation $\circlearrowleft$",
                xy=front_center, xytext=(-3.1, -0.1),
                arrowprops=dict(arrowstyle="->", color='#D84315', lw=2.0,
                                connectionstyle="arc3,rad=-0.15"),
                fontsize=8.5, fontweight='bold', color='#BF360C', ha='center', va='center',
                bbox=dict(boxstyle="round,pad=0.35", facecolor='#FFF3E0', edgecolor='#FFB74D', lw=1.2),
                zorder=8)

    # Plane 2: (X2, X3) on orthogonal hyperface (pointing to hyper-edge B7-B15)
    hyper_pt = ((outer[7][0] + inner[7][0]) * 0.5, (outer[7][1] + inner[7][1]) * 0.5)
    ax.annotate(r"$\mathcal{P}_2: (X_2, X_3)$ Plane" + "\n" + r"Hyperface Rotation $\circlearrowleft$",
                xy=hyper_pt, xytext=(3.25, 0.45),
                arrowprops=dict(arrowstyle="->", color='#2E7D32', lw=2.0,
                                connectionstyle="arc3,rad=0.2"),
                fontsize=8.5, fontweight='bold', color='#1B5E20', ha='center', va='center',
                bbox=dict(boxstyle="round,pad=0.35", facecolor='#E8F5E9', edgecolor='#81C784', lw=1.2),
                zorder=8)

    # 6. Top Legend Box
    legend_elements = [
        Line2D([0], [0], marker='o', color='#1B365D', label=r'Outer Cube ($w=0, B_0 \dots B_7$)',
               markerfacecolor='#1B365D', markeredgecolor='white', markersize=9, lw=2.2),
        Line2D([0], [0], marker='o', color='#00897B', label=r'Inner Cube ($w=1, B_8 \dots B_{15}$)',
               markerfacecolor='#C62828', markeredgecolor='white', markersize=8.5, lw=2.0),
        Line2D([0], [0], color='#8E24AA', lw=2.2, ls='-', label=r'4D Hyper-edges ($w$-axis)')
    ]
    leg = ax.legend(handles=legend_elements, loc='upper center', bbox_to_anchor=(0.5, 1.05),
                    ncol=3, frameon=True, facecolor='#F8F9FA', edgecolor='#B0BEC5', fontsize=8.5,
                    columnspacing=1.2, handletextpad=0.5)
    leg.get_frame().set_linewidth(1.0)
    leg.set_zorder(10)

    # 7. Bottom Title & Mathematical Context Box
    ax.text(0, -2.65, r"16 State Bytes $B_0 \dots B_{15}$ Mapped to 16 Vertices of 4D Tesseract ($\{0, 1\}^4$)",
            ha='center', fontsize=10.5, fontweight='bold', color='#0D47A1')
    ax.text(0, -2.95, r"Orthogonal Double Rotation: $\mathbb{R}^4 = \mathcal{P}_1 \oplus \mathcal{P}_2 \Rightarrow$ Zero Invariant Axis (No Fixed Points)",
            ha='center', fontsize=9.0, style='italic', color='#37474F')

    ax.set_xlim(-4.2, 4.2)
    ax.set_ylim(-3.3, 2.7)

    plt.tight_layout()
    out_path = os.path.join(OUTPUT_DIR, 'fig_tesseract_4d.png')
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Generated: {out_path}")

# ==============================================================================
# FIGURE 2: Rubik-4D Overall Architecture & Round Pipeline (AES Stage Breakdown, B&W Minimalist)
# ==============================================================================
def generate_fig_architecture():
    fig, ax = plt.subplots(figsize=(11.5, 9.5), dpi=300)
    ax.set_xlim(-1.2, 14.8)
    ax.set_ylim(-0.2, 11.8)
    ax.axis('off')

    # Style helper: Minimalist B&W Box
    def draw_box(x, y, w, h, text, subtext="", lw=1.2, ls='-'):
        box = FancyBboxPatch((x, y), w, h, boxstyle="square,pad=0.0",
                             facecolor="#FFFFFF", edgecolor="#000000", lw=lw, ls=ls, zorder=4)
        ax.add_patch(box)
        if subtext:
            ax.text(x + w/2, y + h*0.62, text, ha='center', va='center',
                    fontsize=9.0, fontweight='bold', color="#000000", zorder=5)
            ax.text(x + w/2, y + h*0.28, subtext, ha='center', va='center',
                    fontsize=7.5, color="#000000", zorder=5)
        else:
            ax.text(x + w/2, y + h/2, text, ha='center', va='center',
                    fontsize=9.0, fontweight='bold', color="#000000", zorder=5)

    def draw_circ_step(x, y, num_str, label_lines):
        c = Circle((x, y), 0.28, facecolor='#FFFFFF', edgecolor='#000000', lw=1.3, zorder=6)
        ax.add_patch(c)
        ax.text(x, y, num_str, ha='center', va='center', fontsize=9.5, fontweight='bold', color='#000000', zorder=7)
        if label_lines:
            ax.text(x, y - 0.42, "\n".join(label_lines), ha='center', va='top',
                    fontsize=7.5, fontweight='bold', color="#000000")

    # --------------------------------------------------------------------------
    # TOP SECTION TITLES
    # --------------------------------------------------------------------------
    ax.text(2.1, 11.35, "(a) ENCRYPTION", fontsize=11, fontweight='bold', color='#000000', ha='center')
    ax.text(6.8, 11.35, "KEY EXPANSION", fontsize=11, fontweight='bold', color='#000000', ha='center')
    ax.text(11.5, 11.35, "(b) DECRYPTION", fontsize=11, fontweight='bold', color='#000000', ha='center')

    # --------------------------------------------------------------------------
    # CENTER COLUMN: KEY EXPANSION (AES STYLE)
    # --------------------------------------------------------------------------
    # 1. Passphrase
    draw_box(5.3, 10.2, 3.0, 0.65, "Passphrase", "Variable-length input")
    ax.annotate("", xy=(6.8, 9.4), xytext=(6.8, 10.2),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    # 2. HKDF / Hash
    draw_box(5.1, 8.7, 3.4, 0.7, "HKDF-SHA256", "RFC 5869 KDF")
    ax.annotate("", xy=(6.8, 7.9), xytext=(6.8, 8.7),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    # 3. Master Key K0
    draw_box(5.3, 7.2, 3.0, 0.7, r"Master Key $K_0$", "128-bit [127:0]")
    ax.annotate("", xy=(6.8, 6.2), xytext=(6.8, 7.2),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    # 4. Key Expansion Block
    draw_box(5.1, 4.7, 3.4, 1.5, "Key Expansion",
             r"$K_r = \mathrm{SBox}(K_{r-1}) \oplus \mathrm{Rcon}_r$" + "\n" +
             r"$\mathrm{tbl}_{\mathrm{idx}} = f(r, \bigoplus K_0)$")

    # --------------------------------------------------------------------------
    # LEFT COLUMN: ENCRYPTION (AES 3-STAGE STRUCTURE)
    # --------------------------------------------------------------------------
    # Plaintext Input
    draw_box(0.7, 10.2, 2.8, 0.65, "Plaintext", "128-bit [127:0]")
    ax.annotate("", xy=(2.1, 9.5), xytext=(2.1, 10.2),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    # Stage 1: Initial Round (Key Whitening)
    box_enc_s1 = Rectangle((0.4, 8.45), 3.4, 1.05, fill=False, edgecolor="#000000", lw=1.1, ls="--", zorder=2)
    ax.add_patch(box_enc_s1)
    draw_box(0.7, 8.65, 2.8, 0.65, "AddRoundKey", r"$S^{(0)} = P \oplus K_0$")

    draw_circ_step(-0.4, 9.15, "1", ["Initial", "Round"])

    # Wire from K0 to Enc Stage 1 AddRoundKey
    ax.plot([5.3, 4.2, 4.2], [7.55, 7.55, 8.97], color="#000000", lw=1.1)
    ax.annotate("", xy=(3.5, 8.97), xytext=(4.2, 8.97),
                arrowprops=dict(arrowstyle="->", lw=1.1, color="#000000"))
    ax.text(4.2, 8.2, r"$K_0$", fontsize=8, fontweight='bold', color="#000000", ha='center',
            bbox=dict(boxstyle="square,pad=0.1", facecolor="#FFFFFF", edgecolor="none"))

    # Arrow Stage 1 -> Stage 2
    ax.annotate("", xy=(2.1, 7.65), xytext=(2.1, 8.45),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    # Stage 2: Round Function (8 Rounds)
    box_enc_s2 = Rectangle((0.4, 1.8), 3.4, 5.85, fill=False, edgecolor="#000000", lw=1.2, ls="--", zorder=2)
    ax.add_patch(box_enc_s2)

    draw_circ_step(-0.4, 6.7, "2", ["Rounds", "1 to 8"])

    # Round sub-operations (AES equivalent)
    draw_box(0.7, 6.6, 2.8, 0.65, "SubBytes", "16 AES S-Boxes")
    ax.annotate("", xy=(2.1, 5.95), xytext=(2.1, 6.6),
                arrowprops=dict(arrowstyle="->", lw=1.1, color="#000000"))

    draw_box(0.7, 5.3, 2.8, 0.65, "4D Permutation", r"$SO(4)$ Rotation ($\pi$)")
    ax.annotate("", xy=(2.1, 4.65), xytext=(2.1, 5.3),
                arrowprops=dict(arrowstyle="->", lw=1.1, color="#000000"))

    draw_box(0.7, 4.0, 2.8, 0.65, "ARX Diffusion", "32-bit Ripple-Carry")
    ax.annotate("", xy=(2.1, 3.35), xytext=(2.1, 4.0),
                arrowprops=dict(arrowstyle="->", lw=1.1, color="#000000"))

    draw_box(0.7, 2.7, 2.8, 0.65, "AddRoundKey", r"$S^{(r)} = S \oplus K_r$")

    # Wire from Key Expansion to Enc Round AddRoundKey
    ax.plot([5.1, 4.2, 4.2], [5.45, 5.45, 3.02], color="#000000", lw=1.1)
    ax.annotate("", xy=(3.5, 3.02), xytext=(4.2, 3.02),
                arrowprops=dict(arrowstyle="->", lw=1.1, color="#000000"))
    ax.text(4.2, 4.25, r"$K_r, \pi$", fontsize=8, fontweight='bold', color="#000000", ha='center',
            bbox=dict(boxstyle="square,pad=0.1", facecolor="#FFFFFF", edgecolor="none"))

    # Arrow Stage 2 -> Stage 3
    ax.annotate("", xy=(2.1, 1.15), xytext=(2.1, 1.8),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    # Stage 3: Output Generation
    box_enc_s3 = Rectangle((0.4, 0.05), 3.4, 1.05, fill=False, edgecolor="#000000", lw=1.1, ls="--", zorder=2)
    ax.add_patch(box_enc_s3)
    draw_box(0.7, 0.25, 2.8, 0.65, "Ciphertext", "128-bit [127:0]")

    draw_circ_step(-0.4, 0.6, "3", ["Output"])

    # --------------------------------------------------------------------------
    # RIGHT COLUMN: DECRYPTION (AES DUAL STRUCTURE)
    # --------------------------------------------------------------------------
    # Ciphertext Input
    draw_box(10.1, 10.2, 2.8, 0.65, "Ciphertext", "128-bit [127:0]")
    ax.annotate("", xy=(11.5, 7.25), xytext=(11.5, 10.2),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    # Stage 1: Inverse Rounds (8 Rounds: r = 8 down to 1)
    box_dec_s1 = Rectangle((9.8, 1.8), 3.4, 5.85, fill=False, edgecolor="#000000", lw=1.2, ls="--", zorder=2)
    ax.add_patch(box_dec_s1)

    draw_circ_step(14.0, 6.7, "1", ["Inverse", "Rounds", "8 to 1"])

    draw_box(10.1, 6.6, 2.8, 0.65, "AddRoundKey", r"$S \leftarrow S \oplus K_r$")

    # Wire from Key Expansion to Dec Round AddRoundKey
    ax.plot([8.5, 9.4, 9.4], [5.45, 5.45, 6.92], color="#000000", lw=1.1)
    ax.annotate("", xy=(10.1, 6.92), xytext=(9.4, 6.92),
                arrowprops=dict(arrowstyle="->", lw=1.1, color="#000000"))
    ax.text(9.4, 6.18, r"$K_r$", fontsize=8, fontweight='bold', color="#000000", ha='center',
            bbox=dict(boxstyle="square,pad=0.1", facecolor="#FFFFFF", edgecolor="none"))

    ax.annotate("", xy=(11.5, 5.95), xytext=(11.5, 6.6),
                arrowprops=dict(arrowstyle="->", lw=1.1, color="#000000"))

    draw_box(10.1, 5.3, 2.8, 0.65, "Inv-ARX Diffusion", r"$\boxminus C$, Reverse Words")
    ax.annotate("", xy=(11.5, 4.65), xytext=(11.5, 5.3),
                arrowprops=dict(arrowstyle="->", lw=1.1, color="#000000"))

    draw_box(10.1, 4.0, 2.8, 0.65, "Inv-4D Permutation", r"Inverse Rotation ($\pi^{-1}$)")
    ax.annotate("", xy=(11.5, 3.35), xytext=(11.5, 4.0),
                arrowprops=dict(arrowstyle="->", lw=1.1, color="#000000"))

    draw_box(10.1, 2.7, 2.8, 0.65, "InvSubBytes", r"16 AES $\mathrm{S\text{-}Box}^{-1}$")

    # Arrow Dec Stage 1 -> Dec Stage 2
    ax.annotate("", xy=(11.5, 1.45), xytext=(11.5, 1.8),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    # Stage 2: Output Recovery (Final Inverse Whitening)
    box_dec_s2 = Rectangle((9.8, 0.05), 3.4, 1.6, fill=False, edgecolor="#000000", lw=1.1, ls="--", zorder=2)
    ax.add_patch(box_dec_s2)

    draw_box(10.1, 0.95, 2.8, 0.55, "AddRoundKey", r"$P = S \oplus K_0$")

    # Wire from K0 to Dec Stage 2 AddRoundKey
    ax.plot([8.3, 9.4, 9.4], [7.55, 7.55, 1.22], color="#000000", lw=1.1)
    ax.annotate("", xy=(10.1, 1.22), xytext=(9.4, 1.22),
                arrowprops=dict(arrowstyle="->", lw=1.1, color="#000000"))
    ax.text(9.4, 2.3, r"$K_0$", fontsize=8, fontweight='bold', color="#000000", ha='center',
            bbox=dict(boxstyle="square,pad=0.1", facecolor="#FFFFFF", edgecolor="none"))

    # Arrow AddRoundKey -> Plaintext
    ax.annotate("", xy=(11.5, 0.65), xytext=(11.5, 0.95),
                arrowprops=dict(arrowstyle="->", lw=1.1, color="#000000"))

    draw_box(10.1, 0.15, 2.8, 0.5, "Plaintext", "128-bit [127:0]")

    draw_circ_step(14.0, 0.9, "2", ["Plaintext", "Recovery"])

    plt.tight_layout()
    out_path = os.path.join(OUTPUT_DIR, 'fig_rubik4d_architecture.png')
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Generated: {out_path}")

# ==============================================================================
# FIGURE 3: Key Schedule Generation Flowchart (B&W Minimalist)
# ==============================================================================
def generate_fig_key_schedule():
    fig, ax = plt.subplots(figsize=(11.0, 4.6), dpi=300)
    ax.set_xlim(-0.2, 12.6)
    ax.set_ylim(-0.2, 5.2)
    ax.axis('off')

    def draw_box(x, y, w, h, title, subtitle="", lw=1.2, ls='-'):
        box = FancyBboxPatch((x, y), w, h, boxstyle="square,pad=0.0",
                             facecolor="#FFFFFF", edgecolor="#000000", lw=lw, ls=ls, zorder=3)
        ax.add_patch(box)
        if subtitle:
            ax.text(x + w/2, y + h*0.62, title, ha='center', va='center',
                    fontsize=8.8, fontweight='bold', color="#000000", zorder=4)
            ax.text(x + w/2, y + h*0.28, subtitle, ha='center', va='center',
                    fontsize=7.5, color="#000000", zorder=4)
        else:
            ax.text(x + w/2, y + h/2, title, ha='center', va='center',
                    fontsize=8.8, fontweight='bold', color="#000000", zorder=4)

    # 1. Passphrase Input
    draw_box(0.0, 3.2, 1.8, 1.2, "Passphrase", "Variable-length\nCredentials")

    # Arrow Passphrase -> KDF
    ax.annotate("", xy=(2.2, 3.8), xytext=(1.8, 3.8),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    # 2. Cryptographic Hash / KDF
    draw_box(2.2, 3.2, 1.9, 1.2, "HKDF-SHA256", "RFC 5869 KDF\nEntropy Extraction")

    # Arrow KDF -> Master Key K0
    ax.annotate("", xy=(4.5, 3.8), xytext=(4.1, 3.8),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    # 3. Master Key K0
    draw_box(4.5, 3.2, 1.8, 1.2, r"Master Key $K_0$", "128-bit [127:0]\nNIST Ingestion")

    # Branch 1: Key Folding (Downwards)
    ax.plot([5.4, 5.4], [3.2, 1.6], color="#000000", lw=1.2)
    ax.annotate("", xy=(6.0, 1.6), xytext=(5.4, 1.6),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    draw_box(6.0, 1.0, 2.4, 1.2, "Key Folding",
             r"$k\_fold = \bigoplus_{i=0}^{15} K_0[i]$" + "\n" + r"$\mathrm{tbl}_{\mathrm{idx}} = f(r, k\_fold)$")

    ax.annotate("", xy=(9.0, 1.6), xytext=(8.4, 1.6),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    draw_box(9.0, 1.0, 2.4, 1.2, r"Table Selection",
             r"Active Table $\pi_{\mathrm{tbl}_{\mathrm{idx}}}$" + "\n" + r"1 of 12 $SO(4)$ (192-B LUT)")

    # Branch 2: Recursive Subkey Derivation (Rightwards)
    ax.annotate("", xy=(6.8, 3.8), xytext=(6.3, 3.8),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    draw_box(6.8, 3.2, 1.6, 1.2, "Byte Shift", "Circular Shift by 3\n" + r"$K[(i+3)\,\mathrm{mod}\,16]$")

    ax.annotate("", xy=(8.9, 3.8), xytext=(8.4, 3.8),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    draw_box(8.9, 3.2, 1.6, 1.2, "SubBytes", r"16 AES S-Boxes" + "\n" + r"$\mathrm{GF}(2^8)$ Non-linear")

    # Round Constant Addition (XOR)
    circle_xorc = Circle((10.9, 3.8), 0.22, facecolor="#FFFFFF", edgecolor="#000000", lw=1.2, zorder=4)
    ax.add_patch(circle_xorc)
    ax.text(10.9, 3.8, r"$\oplus$", ha='center', va='center', fontsize=11, fontweight='bold', color="#000000", zorder=5)

    ax.annotate("", xy=(10.68, 3.8), xytext=(10.5, 3.8),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    ax.annotate(r"$\mathrm{Rcon}_r = (r \times \mathrm{0x1B})\,\mathrm{mod}\,256$" + "\nRound Constant",
                xy=(10.9, 4.02), xytext=(10.9, 4.75),
                arrowprops=dict(arrowstyle="->", lw=1.1, color="#000000"),
                ha='center', fontsize=7.5, fontweight='bold', color="#000000")

    # Output Subkey Kr
    ax.annotate("", xy=(11.5, 3.8), xytext=(11.12, 3.8),
                arrowprops=dict(arrowstyle="->", lw=1.2, color="#000000"))

    draw_box(11.45, 3.2, 1.05, 1.2, r"Subkey $K_r$", r"$r = 1..8$" + "\n128-bit")

    plt.tight_layout()
    out_path = os.path.join(OUTPUT_DIR, 'fig_key_schedule.png')
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Generated: {out_path}")

# ==============================================================================
# FIGURE 4: Software Performance Benchmark (Throughput & Latency)
# ==============================================================================
def generate_fig_benchmarks():
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(8.5, 3.6), dpi=300)

    # Subplot 1: Sustained Throughput (MB/s)
    ciphers = ['AES-128\n(SW)', 'Simon-128', 'ChaCha20', 'Rubik-4D\n(Enc)', 'Rubik-4D\n(Dec)', 'Speck-128']
    throughputs = [120.77, 149.20, 155.40, 145.54, 162.28, 514.80]
    colors = ['#78909C', '#5C6BC0', '#26A69A', '#1E88E5', '#0D47A1', '#FFA726']

    bars1 = ax1.bar(ciphers, throughputs, color=colors, width=0.55, edgecolor='#37474F', lw=1.2)
    ax1.set_ylabel('Sustained Throughput (MB/s)', fontweight='bold')
    ax1.set_title('(a) Cipher Throughput Comparison', fontweight='bold', pad=10)
    ax1.grid(axis='y', ls='--', alpha=0.5)
    ax1.set_ylim(0, 600)

    for bar in bars1:
        yval = bar.get_height()
        ax1.text(bar.get_x() + bar.get_width()/2.0, yval + 10, f"{yval:.1f}",
                 ha='center', va='bottom', fontsize=8.5, fontweight='bold')

    # Subplot 2: Clock Cycles per Byte (cpb) - Lower is better!
    lat_ciphers = ['Speck-128', 'Simon-128', 'Rubik-4D\n(Enc)', 'Rubik-4D\n(Dec)', 'AES-128\n(SW)']
    cpb_vals = [5.82, 19.94, 20.20, 21.22, 26.23]
    lat_colors = ['#FFA726', '#5C6BC0', '#1E88E5', '#0D47A1', '#78909C']

    bars2 = ax2.bar(lat_ciphers, cpb_vals, color=lat_colors, width=0.52, edgecolor='#37474F', lw=1.2)
    ax2.set_ylabel('Cycles per Byte (cpb) [Lower is Better]', fontweight='bold')
    ax2.set_title('(b) Single-Block Execution Latency', fontweight='bold', pad=10)
    ax2.grid(axis='y', ls='--', alpha=0.5)
    ax2.set_ylim(0, 32)

    for bar in bars2:
        yval = bar.get_height()
        ax2.text(bar.get_x() + bar.get_width()/2.0, yval + 0.6, f"{yval:.2f}",
                 ha='center', va='bottom', fontsize=8.5, fontweight='bold')

    plt.tight_layout()
    out_path = os.path.join(OUTPUT_DIR, 'fig_benchmark_charts.png')
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Generated: {out_path}")

# ==============================================================================
# FIGURE 5: Avalanche Diffusion & MILP Active S-Box Growth
# ==============================================================================
def generate_fig_avalanche_milp():
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(8.5, 3.6), dpi=300)

    # Subplot 1: Avalanche Bit Flip Ratio across Rounds
    rounds = np.arange(0, 9)
    # Measured avalanche data from analysis.cpp:
    avalanche_pt = [0.78, 12.45, 34.60, 48.92, 50.15, 49.95, 50.04, 50.02, 50.01] # Plaintext 1-bit flip
    avalanche_key = [0.0, 15.20, 36.80, 49.10, 50.22, 49.98, 50.01, 49.99, 50.00] # Key 1-bit flip

    ax1.plot(rounds, avalanche_pt, marker='o', lw=2.0, color='#1E88E5', label='Plaintext Sensitivity')
    ax1.plot(rounds, avalanche_key, marker='s', lw=2.0, color='#E53935', ls='--', label='Key Sensitivity')
    ax1.axhline(50.0, color='#2E7D32', lw=1.5, ls='-.', label='Ideal Strict Avalanche (50%)')
    ax1.fill_between(rounds, 48.0, 52.0, color='#C8E6C9', alpha=0.4, label='Optimal Corridor (48-52%)')

    ax1.set_xlabel('Round Number $r$', fontweight='bold')
    ax1.set_ylabel('Bit Difference Percentage (%)', fontweight='bold')
    ax1.set_title('(a) Avalanche Propagation Dynamic', fontweight='bold', pad=10)
    ax1.set_xticks(rounds)
    ax1.set_ylim(-2, 60)
    ax1.grid(True, ls='--', alpha=0.5)
    ax1.legend(loc='lower right', fontsize=8.5)

    # Subplot 2: Cumulative Active S-Box Lower Bound (MILP)
    rounds_milp = np.arange(1, 9)
    active_sboxes = [1, 3, 5, 9, 13, 17, 21, 22] # from MILP CBC solver
    upper_bound_prob = [-6, -18, -30, -54, -78, -102, -126, -132]

    color_sbox = '#00897B'
    ax2.plot(rounds_milp, active_sboxes, marker='D', lw=2.2, color=color_sbox, label=r'Active S-Boxes $N_{\text{active}}$')
    ax2.set_xlabel('Round Number $r$', fontweight='bold')
    ax2.set_ylabel('Active S-Box Count', color=color_sbox, fontweight='bold')
    ax2.tick_params(axis='y', labelcolor=color_sbox)
    ax2.set_xticks(rounds_milp)
    ax2.set_ylim(0, 26)
    ax2.grid(True, ls='--', alpha=0.5)

    # Threshold line for 128-bit security margin (Requires >= 22 S-boxes)
    ax2.axhline(22, color='#C62828', lw=1.6, ls=':', label=r'Security Margin Bound ($N \geq 22$)')

    for r, n in zip(rounds_milp, active_sboxes):
        ax2.text(r, n + 0.9, f"{n}", ha='center', va='bottom', fontsize=9, fontweight='bold', color=color_sbox)

    ax2.set_title('(b) MILP Active S-Box Lower Bound', fontweight='bold', pad=10)
    ax2.legend(loc='upper left', fontsize=8.5)

    plt.tight_layout()
    out_path = os.path.join(OUTPUT_DIR, 'fig_avalanche_diffusion.png')
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Generated: {out_path}")

# ==============================================================================
# FIGURE 6: Related-Key De-synchronization Mechanism
# ==============================================================================
def generate_fig_related_key():
    fig, ax = plt.subplots(figsize=(8.0, 4.0), dpi=300)
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 5.0)
    ax.axis('off')

    def draw_box(x, y, w, h, title, subtitle, facecol, edgecol, textcol='black'):
        box = FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.08,rounding_size=0.15",
                             facecolor=facecol, edgecolor=edgecol, lw=1.6, zorder=2)
        ax.add_patch(box)
        ax.text(x + w/2, y + h*0.62, title, ha='center', va='center',
                fontsize=9, fontweight='bold', color=textcol, zorder=3)
        ax.text(x + w/2, y + h*0.28, subtitle, ha='center', va='center',
                fontsize=8, color=textcol, zorder=3)

    # Key 1 Instance
    draw_box(0.5, 3.2, 1.8, 1.2, "Master Key K", r"$\mathit{k\_fold} = \bigoplus K[i]$", "#E3F2FD", "#1565C0", "#0D47A1")
    # Key 2 Instance (Related Key)
    draw_box(0.5, 0.8, 1.8, 1.2, "Related Key K*", r"$\Delta K = K \oplus K^* \neq 0$", "#FFEBEE", "#C62828", "#B71C1C")

    # Table Index Selection
    ax.annotate("", xy=(3.2, 3.8), xytext=(2.3, 3.8),
                arrowprops=dict(arrowstyle="->", lw=1.8, color="#1565C0"))
    draw_box(3.2, 3.2, 2.0, 1.2, r"Table Index $\text{tbl}_{\text{idx}}$", r"$\mathit{k\_fold} mod 12 = A$", "#E0F2F1", "#00796B", "#004D40")

    ax.annotate("", xy=(3.2, 1.4), xytext=(2.3, 1.4),
                arrowprops=dict(arrowstyle="->", lw=1.8, color="#C62828"))
    draw_box(3.2, 0.8, 2.0, 1.2, r"Table Index $\text{tbl}'_{\text{idx}}$", r"$\mathit{k\_fold}' mod 12 = B$", "#FFF3E0", "#E65100", "#BF360C")

    # Divergence Indicator
    ax.plot([4.2, 4.2], [3.2, 2.0], color="#D32F2F", lw=2.0, ls='--')
    ax.scatter(4.2, 2.6, s=180, color="#D32F2F", edgecolors='white', lw=1.5, zorder=4)
    ax.text(4.2, 2.6, r"$\neq$", ha='center', va='center', fontsize=12, fontweight='bold', color='white', zorder=5)

    # 4D Permutations Diverge
    ax.annotate("", xy=(6.0, 3.8), xytext=(5.2, 3.8),
                arrowprops=dict(arrowstyle="->", lw=1.8, color="#00796B"))
    draw_box(6.0, 3.2, 2.2, 1.2, r"Permutation $\pi_A$", r"Rotates on Plane $\mathcal{P}_{X_0 X_1}$", "#E8F5E9", "#2E7D32", "#1B5E20")

    ax.annotate("", xy=(6.0, 1.4), xytext=(5.2, 1.4),
                arrowprops=dict(arrowstyle="->", lw=1.8, color="#E65100"))
    draw_box(6.0, 0.8, 2.2, 1.2, r"Permutation $\pi_B$", r"Rotates on Plane $\mathcal{P}_{X_2 X_3}$", "#FCE4EC", "#C2185B", "#880E4F")

    # Immediate Trail Collapse Banner
    collapse_box = FancyBboxPatch((8.5, 1.2), 1.2, 2.8, boxstyle="round,pad=0.08,rounding_size=0.15",
                                  facecolor="#ECEFF1", edgecolor="#37474F", lw=1.8, zorder=2)
    ax.add_patch(collapse_box)
    ax.text(9.1, 2.9, r"$\mathbf{CATACLYSMIC}$" + "\n" + r"$\mathbf{DE-SYNC}$", ha='center', va='center',
            fontsize=8.5, fontweight='bold', color="#C62828")
    ax.text(9.1, 1.8, r"Trails Diverge" + "\n" + r"$\text{Enc}: R_1 \uparrow$" + "\n" + r"$\text{Dec}: R_8 \downarrow$" + "\n" + r"$P_{\text{RK}} \leq 2^{-240}$",
            ha='center', va='center', fontsize=7.5, color="#263238")

    ax.annotate("", xy=(8.5, 3.8), xytext=(8.2, 3.8),
                arrowprops=dict(arrowstyle="->", lw=1.8, color="#2E7D32"))
    ax.annotate("", xy=(8.5, 1.4), xytext=(8.2, 1.4),
                arrowprops=dict(arrowstyle="->", lw=1.8, color="#C2185B"))

    plt.tight_layout()
    out_path = os.path.join(OUTPUT_DIR, 'fig_related_key_desync.png')
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Generated: {out_path}")

if __name__ == '__main__':
    print("Generating academic figures...")
    generate_fig_tesseract()
    generate_fig_architecture()
    generate_fig_key_schedule()
    generate_fig_benchmarks()
    generate_fig_avalanche_milp()
    generate_fig_related_key()
    print("All figures successfully generated!")
