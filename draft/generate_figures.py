import matplotlib.pyplot as plt
import matplotlib.patches as patches
from matplotlib.patches import FancyBboxPatch, ArrowStyle
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
    fig = plt.figure(figsize=(7, 6), dpi=300)
    ax = fig.add_subplot(111)
    ax.set_aspect('equal')
    ax.axis('off')

    # Define 16 vertices of a 4D tesseract projected into 2D:
    # Outer cube vertices (w = 0) and Inner cube vertices (w = 1)
    # Using stereographic/isometric perspective:
    scale_out = 2.4
    scale_in = 1.15
    center = (0, 0)

    # 8 vertices for a cube: (x, y, z) in {-1, 1}^3 projected to 2D
    # projection: X = x + 0.45*z, Y = y + 0.35*z
    def project_cube(s, offset_x=0, offset_y=0):
        coords = []
        for x in [-1, 1]:
            for y in [-1, 1]:
                for z in [-1, 1]:
                    X = (x + 0.45 * z) * s * 0.7 + offset_x
                    Y = (y + 0.35 * z) * s * 0.7 + offset_y
                    coords.append((X, Y))
        return coords

    outer = project_cube(scale_out)
    inner = project_cube(scale_in)
    all_verts = outer + inner

    # Edges within outer cube (12 edges)
    # (x, y, z): indices 0:(-1,-1,-1), 1:(-1,-1,1), 2:(-1,1,-1), 3:(-1,1,1),
    #            4:(1,-1,-1), 5:(1,-1,1), 6:(1,1,-1), 7:(1,1,1)
    cube_edges = [
        (0, 1), (2, 3), (4, 5), (6, 7), # z-direction
        (0, 2), (1, 3), (4, 6), (5, 7), # y-direction
        (0, 4), (1, 5), (2, 6), (3, 7)  # x-direction
    ]

    # Draw outer cube edges (Thick dark blue)
    for u, v in cube_edges:
        ax.plot([outer[u][0], outer[v][0]], [outer[u][1], outer[v][1]],
                color='#1B365D', lw=1.8, zorder=2)

    # Draw inner cube edges (Teal)
    for u, v in cube_edges:
        ax.plot([inner[u][0], inner[v][0]], [inner[u][1], inner[v][1]],
                color='#008080', lw=1.6, zorder=2)

    # Draw 8 connecting 4D hyper-edges between outer and inner cubes (Dashed purple)
    for i in range(8):
        ax.plot([outer[i][0], inner[i][0]], [outer[i][1], inner[i][1]],
                color='#7B1FA2', lw=1.4, ls='--', alpha=0.85, zorder=1)

    # Annotate 16 vertices with Byte indices B0 to B15 and binary coordinates
    # Outer cube: B0..B7 (w = 0)
    # Inner cube: B8..B15 (w = 1)
    for i in range(8):
        # Outer
        px, py = outer[i]
        ax.scatter(px, py, s=260, color='#1B365D', edgecolors='white', lw=1.5, zorder=4)
        ax.text(px, py, f"$B_{{{i}}}$", color='white', ha='center', va='center',
                fontsize=8.5, fontweight='bold', zorder=5)
        # Inner
        qx, qy = inner[i]
        ax.scatter(qx, qy, s=240, color='#C62828', edgecolors='white', lw=1.5, zorder=4)
        ax.text(qx, qy, f"$B_{{{i+8}}}$", color='white', ha='center', va='center',
                fontsize=8.5, fontweight='bold', zorder=5)

    # Draw illustration of SO(4) double rotation planes
    # Plane 1: (X-Y) on outer face
    arc1 = patches.Arc((-1.1, -1.0), 1.2, 1.2, angle=0, theta1=20, theta2=160,
                       color='#D84315', lw=2.2, ls='-', zorder=6)
    ax.add_patch(arc1)
    ax.annotate(r"$\mathcal{P}_1: (X_0, X_1)$ Plane", xy=(-1.1, -0.4), xytext=(-2.3, -0.2),
                arrowprops=dict(arrowstyle="->", color='#D84315', lw=1.5),
                fontsize=9.5, fontweight='bold', color='#D84315',
                bbox=dict(boxstyle="round,pad=0.2", facecolor='#FFF3E0', edgecolor='#FFB74D'))

    # Plane 2: (Z-W) on inner/outer connecting face
    arc2 = patches.Arc((0.6, 0.4), 1.0, 1.0, angle=45, theta1=30, theta2=180,
                       color='#2E7D32', lw=2.2, ls='-', zorder=6)
    ax.add_patch(arc2)
    ax.annotate(r"$\mathcal{P}_2: (X_2, X_3)$ Plane", xy=(0.8, 0.9), xytext=(1.4, 1.3),
                arrowprops=dict(arrowstyle="->", color='#2E7D32', lw=1.5),
                fontsize=9.5, fontweight='bold', color='#2E7D32',
                bbox=dict(boxstyle="round,pad=0.2", facecolor='#E8F5E9', edgecolor='#A5D6A7'))

    # Title & Legend Box
    ax.text(0, -2.45, r"16 State Bytes $B_0 \dots B_{15}$ Mapped to 16 Vertices of 4D Tesseract ($\{0, 1\}^4$)",
            ha='center', fontsize=10.5, fontweight='bold', color='#1A237E')
    ax.text(0, -2.75, r"Orthogonal Double Rotation: $\mathbb{R}^4 = \mathcal{P}_1 \oplus \mathcal{P}_2 \Rightarrow$ Zero Invariant Axis (No Fixed Points)",
            ha='center', fontsize=9, style='italic', color='#37474F')

    plt.tight_layout()
    out_path = os.path.join(OUTPUT_DIR, 'fig_tesseract_4d.png')
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Generated: {out_path}")

# ==============================================================================
# FIGURE 2: Rubik-4D Overall Architecture & Round Pipeline
# ==============================================================================
def generate_fig_architecture():
    fig, ax = plt.subplots(figsize=(8.5, 4.6), dpi=300)
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 5.5)
    ax.axis('off')

    def draw_box(x, y, w, h, title, subtitle, facecol, edgecol, textcol='black'):
        box = FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.08,rounding_size=0.15",
                             facecolor=facecol, edgecolor=edgecol, lw=1.6, zorder=2)
        ax.add_patch(box)
        ax.text(x + w/2, y + h*0.62, title, ha='center', va='center',
                fontsize=9.5, fontweight='bold', color=textcol, zorder=3)
        ax.text(x + w/2, y + h*0.28, subtitle, ha='center', va='center',
                fontsize=8, color=textcol, zorder=3)

    # Input Block
    draw_box(0.3, 2.2, 1.4, 1.0, "Plaintext Block P", "128-bit (16 Bytes)", "#ECEFF1", "#455A64")

    # Initial XOR with Round Key K0
    ax.annotate("", xy=(2.3, 2.7), xytext=(1.7, 2.7),
                arrowprops=dict(arrowstyle="->", lw=1.8, color="#37474F"))
    circle_xor0 = patches.Circle((2.55, 2.7), 0.25, facecolor="#FFF9C4", edgecolor="#F57F17", lw=1.5, zorder=2)
    ax.add_patch(circle_xor0)
    ax.text(2.55, 2.7, r"$\oplus$", ha='center', va='center', fontsize=12, fontweight='bold', color="#E65100")
    ax.annotate(r"Whitening $K_0$", xy=(2.55, 2.95), xytext=(2.55, 3.8),
                arrowprops=dict(arrowstyle="->", lw=1.5, color="#E65100"),
                ha='center', fontsize=8.5, fontweight='bold', color="#E65100")

    # Round Loop Container
    round_box = FancyBboxPatch((3.1, 0.6), 5.3, 4.2, boxstyle="round,pad=0.1,rounding_size=0.2",
                               facecolor="#F8F9FA", edgecolor="#1976D2", lw=2.0, ls='--', zorder=1)
    ax.add_patch(round_box)
    ax.text(5.75, 4.5, r"$\mathbf{Iterative\; Round\; Function\; (Rounds\; r = 1 \dots 8)}$",
            ha='center', fontsize=10.5, color="#0D47A1", fontweight='bold')

    # Arrow entering round
    ax.annotate("", xy=(3.3, 2.7), xytext=(2.8, 2.7),
                arrowprops=dict(arrowstyle="->", lw=1.8, color="#37474F"))

    # Stage 1: SubBytes
    draw_box(3.4, 2.1, 1.5, 1.2, "1. SubBytes", "16 AES S-Boxes\n" + r"($p_{\max}=2^{-6}, d=7$)",
             "#FFEBEE", "#D32F2F", "#B71C1C")

    # Arrow 1->2
    ax.annotate("", xy=(5.2, 2.7), xytext=(4.9, 2.7),
                arrowprops=dict(arrowstyle="->", lw=1.8, color="#37474F"))

    # Stage 2: SO(4) Permutation
    draw_box(5.2, 2.1, 1.6, 1.2, r"2. $SO(4)$ Rotation", "Tesseract Permutation\n" + r"$\pi_{\text{tbl}_{\text{idx}}}$ (192-B LUT)",
             "#E0F2F1", "#00897B", "#004D40")

    # Arrow 2->3
    ax.annotate("", xy=(7.1, 2.7), xytext=(6.8, 2.7),
                arrowprops=dict(arrowstyle="->", lw=1.8, color="#37474F"))

    # Stage 3: ARX Diffusion
    draw_box(7.1, 2.1, 1.1, 1.2, "3. ARX 32-bit", r"$\boxplus C, \lll n_k, \oplus$" + "\nRipple Diffusion",
             "#FFF3E0", "#FB8C00", "#E65100")

    # Stage 4: AddRoundKey inside round
    circle_xorr = patches.Circle((7.65, 1.2), 0.22, facecolor="#FFF9C4", edgecolor="#F57F17", lw=1.5, zorder=2)
    ax.add_patch(circle_xorr)
    ax.text(7.65, 1.2, r"$\oplus$", ha='center', va='center', fontsize=11, fontweight='bold', color="#E65100")
    ax.annotate(r"Subkey $K_r$", xy=(7.65, 1.2), xytext=(6.3, 1.2),
                arrowprops=dict(arrowstyle="->", lw=1.5, color="#E65100"),
                ha='right', va='center', fontsize=8.5, fontweight='bold', color="#E65100")

    # Path from ARX down to AddRoundKey
    ax.plot([7.65, 7.65], [2.1, 1.42], color="#37474F", lw=1.8)
    ax.annotate("", xy=(7.65, 1.42), xytext=(7.65, 1.6),
                arrowprops=dict(arrowstyle="->", lw=1.8, color="#37474F"))

    # Path from AddRoundKey exiting round or looping back
    # Loop back arrow (for r < 8)
    ax.plot([7.65, 7.65, 3.25, 3.25], [0.98, 0.8, 0.8, 2.7], color="#1976D2", lw=1.4, ls=":")
    ax.annotate("", xy=(3.3, 2.7), xytext=(3.25, 2.5),
                arrowprops=dict(arrowstyle="->", lw=1.4, color="#1976D2"))
    ax.text(5.4, 0.9, r"Feedback for Rounds $r = 1 \dots 7$", fontsize=8, color="#1976D2", ha='center')

    # Exit arrow to Ciphertext (after Round 8)
    ax.annotate("", xy=(9.0, 1.2), xytext=(7.87, 1.2),
                arrowprops=dict(arrowstyle="->", lw=1.8, color="#37474F"))
    draw_box(8.9, 0.7, 1.0, 1.0, "Ciphertext C", "128-bit Block", "#ECEFF1", "#455A64")

    plt.tight_layout()
    out_path = os.path.join(OUTPUT_DIR, 'fig_rubik4d_architecture.png')
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Generated: {out_path}")

# ==============================================================================
# FIGURE 3: Key Schedule Generation Flowchart
# ==============================================================================
def generate_fig_key_schedule():
    fig, ax = plt.subplots(figsize=(8.0, 4.2), dpi=300)
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

    # Master Key Input
    draw_box(0.3, 2.8, 1.8, 1.2, "Master Key K", "128-bit (16 Bytes)\nDirect NIST Model", "#E3F2FD", "#1565C0", "#0D47A1")

    # Branch 1: Key Folding to Table Selection
    ax.plot([1.2, 1.2], [2.8, 1.5], color="#1565C0", lw=1.6)
    ax.annotate("", xy=(2.0, 1.5), xytext=(1.2, 1.5),
                arrowprops=dict(arrowstyle="->", lw=1.6, color="#1565C0"))
    draw_box(2.0, 0.9, 2.2, 1.2, "Cumulative Key Folding", r"$k\_fold = \bigoplus_{i=0}^{15} K[i]$" + "\n" + r"$\text{tbl}_{\text{idx}} = k\_fold mod 12$",
             "#E8F5E9", "#2E7D32", "#1B5E20")

    ax.annotate("", xy=(5.2, 1.5), xytext=(4.2, 1.5),
                arrowprops=dict(arrowstyle="->", lw=1.6, color="#2E7D32"))
    draw_box(5.2, 0.9, 2.4, 1.2, r"Table Selection $\pi_{\text{tbl}}$", "Selects 1 of 12 $SO(4)$\nRotation Configurations",
             "#E0F2F1", "#00695C", "#004D40")

    # Branch 2: Recursive Subkey Derivation
    ax.annotate("", xy=(3.0, 3.4), xytext=(2.1, 3.4),
                arrowprops=dict(arrowstyle="->", lw=1.6, color="#1565C0"))
    draw_box(3.0, 2.8, 1.8, 1.2, "Byte Rotation", "Circular Shift by 3\n" + r"$K[(i+3) mod 16]$",
             "#FFF3E0", "#E65100", "#BF360C")

    ax.annotate("", xy=(5.5, 3.4), xytext=(4.8, 3.4),
                arrowprops=dict(arrowstyle="->", lw=1.6, color="#E65100"))
    draw_box(5.5, 2.8, 1.8, 1.2, "Non-Linear S-Boxes", "16 Parallel AES S-Boxes\n" + r"$\text{S-Box}(\cdot)$ over $\text{GF}(2^8)$",
             "#FFEBEE", "#C62828", "#B71C1C")

    # Round Constant Addition
    ax.annotate("", xy=(7.9, 3.4), xytext=(7.3, 3.4),
                arrowprops=dict(arrowstyle="->", lw=1.6, color="#C62828"))
    circle_xorc = patches.Circle((8.1, 3.4), 0.22, facecolor="#FFF9C4", edgecolor="#F57F17", lw=1.5, zorder=2)
    ax.add_patch(circle_xorc)
    ax.text(8.1, 3.4, r"$\oplus$", ha='center', va='center', fontsize=11, fontweight='bold', color="#E65100")
    ax.annotate(r"Round Constant: $(r \times \text{0x1B}) mod 256$", xy=(8.1, 3.62), xytext=(8.1, 4.4),
                arrowprops=dict(arrowstyle="->", lw=1.4, color="#F57F17"),
                ha='center', fontsize=8, fontweight='bold', color="#E65100")

    # Output Subkey Kr
    ax.annotate("", xy=(9.0, 3.4), xytext=(8.32, 3.4),
                arrowprops=dict(arrowstyle="->", lw=1.6, color="#37474F"))
    draw_box(8.9, 2.8, 1.0, 1.2, r"Subkey $K_r$", r"$r \in \{1 \dots 8\}$" + "\n128-bit",
             "#ECEFF1", "#37474F", "#263238")

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
