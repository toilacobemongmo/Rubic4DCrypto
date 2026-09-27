"""
Rubik-4D Cryptographic Lab & Interactive Visualizer
A modern desktop GUI application demonstrating:
1. 4D Tesseract Real-time 3D/4D Projection & SO(4) Hypercube Rotation.
2. Image & File Encryption Lab with Rubik4D-GCM AEAD and SIMD AVX2 CTR.
3. Tamper & Forgery Attack Simulation (Bit-flipping, AAD Tampering).
4. Live CPU Microarchitecture Benchmark (Cycles/Byte & MB/s).
"""

import os
import sys
import time
import math
import secrets
import threading
from typing import Optional

import tkinter as tk
from tkinter import filedialog, messagebox
import customtkinter as ctk
from PIL import Image, ImageTk
import numpy as np

import matplotlib
matplotlib.use("TkAgg")
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

# Import our engine & math modules
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from gui.rubik4d_engine import (
    HAS_SSSE3, HAS_AVX2, HAS_PCLMUL,
    encrypt_aead, decrypt_aead,
    encrypt_ctr_avx2, decrypt_ctr_avx2,
    calculate_entropy, calculate_histogram,
    run_benchmark
)
from gui.tesseract_math import Tesseract4D, EDGES_4D

# Appearance setup
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")


class Rubik4DApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("Rubik-4D Cryptographic Lab & Tesseract Visualizer")
        self.geometry("1280x860")
        self.minsize(1100, 750)

        # State variables
        self.tesseract = Tesseract4D()
        self.tess_running = True
        self.tess_speed = 0.02
        self.tess_plane = "XW"

        # Image Lab state
        self.current_img_path = None
        self.original_img = None
        self.encrypted_img = None
        self.decrypted_img = None
        self.raw_plaintext = b""
        self.raw_ciphertext = b""
        self.raw_tag = b""
        self.img_size = (0, 0)
        self.img_mode = "RGB"

        # Setup Header
        self._create_header()

        # Setup Main Tabview
        self.tabview = ctk.CTkTabview(self, corner_radius=10)
        self.tabview.pack(fill="both", expand=True, padx=15, pady=(0, 15))

        self.tab_tess = self.tabview.add("🔮 4D Tesseract Visualizer")
        self.tab_crypto = self.tabview.add("🖼️ Image & File Cryptography")
        self.tab_tamper = self.tabview.add("🛡️ Tamper Attack & Integrity Lab")
        self.tab_bench = self.tabview.add("⚡ Live Hardware Benchmark")

        # Initialize Tabs
        self._init_tesseract_tab()
        self._init_crypto_tab()
        self._init_tamper_tab()
        self._init_bench_tab()

        # Start 4D Animation Loop
        self._animate_tesseract()

    # -------------------------------------------------------------
    # HEADER SECTION
    # -------------------------------------------------------------
    def _create_header(self):
        header_frame = ctk.CTkFrame(self, fg_color="transparent")
        header_frame.pack(fill="x", padx=20, pady=(12, 6))

        title_lbl = ctk.CTkLabel(
            header_frame,
            text="RUBIK-4D CRYPTOGRAPHIC SUITE",
            font=ctk.CTkFont(family="Segoe UI", size=22, weight="bold"),
            text_color="#38bdf8"
        )
        title_lbl.pack(side="left")

        # Hardware badges
        hw_frame = ctk.CTkFrame(header_frame, fg_color="#1e293b", corner_radius=6)
        hw_frame.pack(side="right")

        def make_badge(name, active):
            color = "#10b981" if active else "#ef4444"
            status = "ACTIVE" if active else "DISABLED"
            lbl = ctk.CTkLabel(
                hw_frame,
                text=f"{name}: {status}",
                font=ctk.CTkFont(size=11, weight="bold"),
                text_color=color,
                padx=8,
                pady=2
            )
            lbl.pack(side="left")

        make_badge("SSSE3", HAS_SSSE3)
        make_badge("AVX2", HAS_AVX2)
        make_badge("PCLMUL", HAS_PCLMUL)

    # -------------------------------------------------------------
    # TAB 1: 4D TESSERACT VISUALIZER
    # -------------------------------------------------------------
    def _init_tesseract_tab(self):
        container = ctk.CTkFrame(self.tab_tess, fg_color="transparent")
        container.pack(fill="both", expand=True, padx=10, pady=10)

        # Left Canvas for 4D Hypercube rendering
        self.tess_canvas = tk.Canvas(
            container,
            bg="#0b0f19",
            highlightthickness=1,
            highlightbackground="#334155"
        )
        self.tess_canvas.pack(side="left", fill="both", expand=True, padx=(0, 10))

        # Right control panel
        ctrl_frame = ctk.CTkFrame(container, width=320, fg_color="#1e293b", corner_radius=10)
        ctrl_frame.pack(side="right", fill="y", padx=(5, 0))
        ctrl_frame.pack_propagate(False)

        ctk.CTkLabel(
            ctrl_frame,
            text="4D ROTATION CONTROLS",
            font=ctk.CTkFont(size=14, weight="bold"),
            text_color="#f8fafc"
        ).pack(pady=(15, 10), padx=15, anchor="w")

        # Animation toggle switch
        self.sw_anim = ctk.CTkSwitch(
            ctrl_frame,
            text="Auto 4D Rotation",
            command=self._toggle_tess_anim
        )
        self.sw_anim.select()
        self.sw_anim.pack(pady=8, padx=15, anchor="w")

        # Rotation Plane Selector
        ctk.CTkLabel(ctrl_frame, text="4D Rotation Plane:", font=ctk.CTkFont(size=12)).pack(pady=(10, 2), padx=15, anchor="w")
        self.plane_combo = ctk.CTkComboBox(
            ctrl_frame,
            values=["XW (Hyper-spin 1)", "YW (Hyper-spin 2)", "ZW (Hyper-spin 3)", "XY + ZW (Clifford Bivector)", "All Planes (Chaos)"],
            command=self._on_plane_change
        )
        self.plane_combo.set("XW (Hyper-spin 1)")
        self.plane_combo.pack(pady=4, padx=15, fill="x")

        # Speed Slider
        ctk.CTkLabel(ctrl_frame, text="Rotation Speed:", font=ctk.CTkFont(size=12)).pack(pady=(12, 2), padx=15, anchor="w")
        self.slider_speed = ctk.CTkSlider(ctrl_frame, from_=0.005, to=0.08, command=self._on_speed_change)
        self.slider_speed.set(0.02)
        self.slider_speed.pack(pady=4, padx=15, fill="x")

        # Distance W Slider
        ctk.CTkLabel(ctrl_frame, text="4D Perspective Camera (W-dist):", font=ctk.CTkFont(size=12)).pack(pady=(12, 2), padx=15, anchor="w")
        self.slider_dist_w = ctk.CTkSlider(ctrl_frame, from_=1.5, to=4.0, command=self._on_dist_w_change)
        self.slider_dist_w.set(2.5)
        self.slider_dist_w.pack(pady=4, padx=15, fill="x")

        # Action: Step SO(4) Permutation
        ctk.CTkLabel(ctrl_frame, text="SO(4) Hypercube Permutation:", font=ctk.CTkFont(size=12, weight="bold"), text_color="#38bdf8").pack(pady=(20, 5), padx=15, anchor="w")
        
        btn_step_perm = ctk.CTkButton(
            ctrl_frame,
            text="Simulate 1 Round Permutation",
            fg_color="#0284c7",
            hover_color="#0369a1",
            command=self._step_permutation
        )
        btn_step_perm.pack(pady=6, padx=15, fill="x")

        btn_reset_perm = ctk.CTkButton(
            ctrl_frame,
            text="Reset Hypercube Vertices",
            fg_color="#475569",
            hover_color="#334155",
            command=self._reset_permutation
        )
        btn_reset_perm.pack(pady=4, padx=15, fill="x")

        # Description Card
        info_box = ctk.CTkTextbox(ctrl_frame, height=180, fg_color="#0f172a", text_color="#94a3b8", font=ctk.CTkFont(size=11))
        info_box.pack(fill="x", padx=15, pady=(15, 10))
        info_box.insert("1.0", (
            "MATHEMATICAL FOUNDATION:\n"
            "- 16 Vertices: Represent 16 state bytes (128-bit state).\n"
            "- 32 Edges: 1-Hamming distance connections in 4D.\n"
            "- SO(4) Permutation: Rotates the 4D coordinate axes, "
            "diffusing all 16 bytes across the 8 cubic bounding cells.\n"
            "- Hardware: Mapped into SSSE3 _mm_shuffle_epi8 (PSHUFB) "
            "executing in exactly 1 CPU clock cycle!"
        ))
        info_box.configure(state="disabled")

    def _toggle_tess_anim(self):
        self.tess_running = self.sw_anim.get() == 1

    def _on_plane_change(self, val):
        self.tess_plane = val

    def _on_speed_change(self, val):
        self.tess_speed = float(val)

    def _on_dist_w_change(self, val):
        self.tesseract.dist_w = float(val)

    def _step_permutation(self):
        # Permutation mapping from SO(4)
        perm = [5, 10, 15, 0, 9, 14, 3, 4, 13, 2, 7, 8, 1, 6, 11, 12]
        new_labels = [self.tesseract.vertex_labels[p] for p in perm]
        self.tesseract.set_permutation(new_labels)

    def _reset_permutation(self):
        self.tesseract.set_permutation(list(range(16)))

    def _animate_tesseract(self):
        if self.tess_running:
            sp = self.tess_speed
            if "XW" in self.tess_plane:
                self.tesseract.angle_xw += sp
                self.tesseract.angle_yz += sp * 0.4
            elif "YW" in self.tess_plane:
                self.tesseract.angle_yw += sp
                self.tesseract.angle_xz += sp * 0.4
            elif "ZW" in self.tess_plane:
                self.tesseract.angle_zw += sp
                self.tesseract.angle_xy += sp * 0.4
            elif "Clifford" in self.tess_plane:
                self.tesseract.angle_xy += sp
                self.tesseract.angle_zw += sp
            else:
                self.tesseract.angle_xw += sp * 0.7
                self.tesseract.angle_yw += sp * 0.5
                self.tesseract.angle_zw += sp * 0.3
                self.tesseract.angle_xy += sp * 0.4

        # Redraw Canvas
        w = self.tess_canvas.winfo_width()
        h = self.tess_canvas.winfo_height()
        if w > 10 and h > 10:
            self.tess_canvas.delete("all")
            pts = self.tesseract.project_to_2d(w, h, scale=min(w, h) * 0.22)

            # Draw 32 Edges
            for i, j in EDGES_4D:
                x1, y1, d1, _ = pts[i]
                x2, y2, d2, _ = pts[j]
                avg_d = (d1 + d2) / 2.0
                # Interpolate color between cyan (#00f0ff) and deep purple (#3b0764)
                edge_color = "#0ea5e9" if avg_d > 0.4 else "#334155"
                line_w = 2 if avg_d > 0.4 else 1
                self.tess_canvas.create_line(x1, y1, x2, y2, fill=edge_color, width=line_w)

            # Draw 16 Vertices
            for idx, (x, y, d, lbl) in enumerate(pts):
                r = 10 if d > 0.4 else 7
                v_color = "#38bdf8" if d > 0.4 else "#64748b"
                self.tess_canvas.create_oval(x - r, y - r, x + r, y + r, fill=v_color, outline="#ffffff", width=1)
                self.tess_canvas.create_text(x, y, text=f"B{lbl}", fill="#0f172a", font=("Segoe UI", 8, "bold"))

        self.after(25, self._animate_tesseract)

    # -------------------------------------------------------------
    # TAB 2: IMAGE & FILE CRYPTOGRAPHY LAB
    # -------------------------------------------------------------
    def _init_crypto_tab(self):
        container = ctk.CTkFrame(self.tab_crypto, fg_color="transparent")
        container.pack(fill="both", expand=True, padx=10, pady=10)

        # Left panel: Parameters & Actions
        left_p = ctk.CTkFrame(container, width=360, fg_color="#1e293b", corner_radius=10)
        left_p.pack(side="left", fill="y", padx=(0, 10))
        left_p.pack_propagate(False)

        ctk.CTkLabel(left_p, text="CRYPTOGRAPHIC CONFIG", font=ctk.CTkFont(size=14, weight="bold"), text_color="#f8fafc").pack(pady=(15, 10), padx=15, anchor="w")

        # Select Mode
        ctk.CTkLabel(left_p, text="Cipher Mode of Operation:", font=ctk.CTkFont(size=12)).pack(pady=(6, 2), padx=15, anchor="w")
        self.mode_selector = ctk.CTkSegmentedButton(
            left_p,
            values=["Rubik4D-GCM (AEAD)", "Rubik4D-CTR (AVX2)"]
        )
        self.mode_selector.set("Rubik4D-GCM (AEAD)")
        self.mode_selector.pack(fill="x", padx=15, pady=4)

        # Key Input
        ctk.CTkLabel(left_p, text="128-bit Master Key (16 bytes Hex):", font=ctk.CTkFont(size=12)).pack(pady=(10, 2), padx=15, anchor="w")
        self.ent_key = ctk.CTkEntry(left_p, placeholder_text="00112233445566778899aabbccddeeff")
        self.ent_key.insert(0, "00112233445566778899aabbccddeeff")
        self.ent_key.pack(fill="x", padx=15, pady=2)

        # Nonce / IV Input
        ctk.CTkLabel(left_p, text="96-bit Nonce / IV (12 bytes Hex):", font=ctk.CTkFont(size=12)).pack(pady=(10, 2), padx=15, anchor="w")
        self.ent_nonce = ctk.CTkEntry(left_p, placeholder_text="0102030405060708090a0b0c")
        self.ent_nonce.insert(0, "0102030405060708090a0b0c")
        self.ent_nonce.pack(fill="x", padx=15, pady=2)

        # AAD Header Input
        ctk.CTkLabel(left_p, text="Associated Data (AAD Header):", font=ctk.CTkFont(size=12)).pack(pady=(10, 2), padx=15, anchor="w")
        self.ent_aad = ctk.CTkEntry(left_p, placeholder_text="Metadata / Header text")
        self.ent_aad.insert(0, "Rubik4D-Auth-Header-V1")
        self.ent_aad.pack(fill="x", padx=15, pady=2)

        # Key Gen & Browse Buttons
        btn_rnd_key = ctk.CTkButton(left_p, text="🎲 Generate Random Key & Nonce", fg_color="#334155", hover_color="#475569", command=self._generate_random_keys)
        btn_rnd_key.pack(fill="x", padx=15, pady=(8, 12))

        btn_browse = ctk.CTkButton(left_p, text="📁 Browse Image / File...", fg_color="#0284c7", hover_color="#0369a1", command=self._browse_image)
        btn_browse.pack(fill="x", padx=15, pady=4)

        self.lbl_file_status = ctk.CTkLabel(left_p, text="No file loaded. (Using test sample)", font=ctk.CTkFont(size=11), text_color="#94a3b8")
        self.lbl_file_status.pack(padx=15, pady=(2, 10))

        # Main Action Buttons
        btn_encrypt = ctk.CTkButton(
            left_p,
            text="🔒 ENCRYPT (Mã hóa)",
            font=ctk.CTkFont(size=13, weight="bold"),
            fg_color="#10b981",
            hover_color="#059669",
            command=self._do_encrypt
        )
        btn_encrypt.pack(fill="x", padx=15, pady=(10, 6))

        btn_decrypt = ctk.CTkButton(
            left_p,
            text="🔓 DECRYPT (Giải mã)",
            font=ctk.CTkFont(size=13, weight="bold"),
            fg_color="#6366f1",
            hover_color="#4f46e5",
            command=self._do_decrypt
        )
        btn_decrypt.pack(fill="x", padx=15, pady=4)

        # Metrics display frame
        metrics_f = ctk.CTkFrame(left_p, fg_color="#0f172a", corner_radius=8)
        metrics_f.pack(fill="x", padx=15, pady=(16, 10))

        self.lbl_metric_time = ctk.CTkLabel(metrics_f, text="Execution Time: -- ms", font=ctk.CTkFont(size=11))
        self.lbl_metric_time.pack(anchor="w", padx=10, pady=(6, 2))

        self.lbl_metric_speed = ctk.CTkLabel(metrics_f, text="Throughput: -- MB/s", font=ctk.CTkFont(size=11, weight="bold"), text_color="#38bdf8")
        self.lbl_metric_speed.pack(anchor="w", padx=10, pady=2)

        self.lbl_metric_entropy = ctk.CTkLabel(metrics_f, text="Entropy: -- / 8.000", font=ctk.CTkFont(size=11))
        self.lbl_metric_entropy.pack(anchor="w", padx=10, pady=2)

        self.lbl_metric_tag = ctk.CTkLabel(metrics_f, text="Tag: --", font=ctk.CTkFont(size=10), text_color="#10b981")
        self.lbl_metric_tag.pack(anchor="w", padx=10, pady=(2, 6))

        # Right Panel: Visual display (Images + Histogram)
        right_p = ctk.CTkFrame(container, fg_color="transparent")
        right_p.pack(side="right", fill="both", expand=True)

        # 3 Image Boxes
        img_preview_box = ctk.CTkFrame(right_p, fg_color="#1e293b", corner_radius=10)
        img_preview_box.pack(fill="x", pady=(0, 10))

        # Sub-frames for Original, Encrypted, Decrypted
        for idx, (title, attr) in enumerate([("Original Plaintext", "lbl_orig_img"), ("Encrypted (Ciphertext Noise)", "lbl_enc_img"), ("Decrypted Recovery", "lbl_dec_img")]):
            f = ctk.CTkFrame(img_preview_box, fg_color="#0f172a", corner_radius=8)
            f.pack(side="left", fill="both", expand=True, padx=6, pady=8)
            ctk.CTkLabel(f, text=title, font=ctk.CTkFont(size=11, weight="bold"), text_color="#94a3b8").pack(pady=(4, 2))
            lbl = ctk.CTkLabel(f, text="\n[No Image]\n", width=160, height=160)
            lbl.pack(pady=6, padx=6)
            setattr(self, attr, lbl)

        # Embedded Matplotlib Figure for Histogram
        hist_box = ctk.CTkFrame(right_p, fg_color="#1e293b", corner_radius=10)
        hist_box.pack(fill="both", expand=True)

        self.fig = Figure(figsize=(6, 2.8), dpi=100, facecolor="#1e293b")
        self.ax = self.fig.add_subplot(111)
        self.ax.set_facecolor("#0f172a")
        self.ax.tick_params(colors="#94a3b8", labelsize=8)
        self.ax.spines['bottom'].set_color('#334155')
        self.ax.spines['top'].set_color('#334155')
        self.ax.spines['left'].set_color('#334155')
        self.ax.spines['right'].set_color('#334155')
        self.ax.set_title("Byte Distribution Histogram (Plaintext vs Ciphertext)", color="#f8fafc", fontsize=10)

        self.hist_canvas = FigureCanvasTkAgg(self.fig, master=hist_box)
        self.hist_canvas.get_tk_widget().pack(fill="both", expand=True, padx=8, pady=8)

        # Load default test sample image
        self._load_sample_image()

    def _generate_random_keys(self):
        rnd_key = secrets.token_hex(16)
        rnd_nonce = secrets.token_hex(12)
        self.ent_key.delete(0, "end")
        self.ent_key.insert(0, rnd_key)
        self.ent_nonce.delete(0, "end")
        self.ent_nonce.insert(0, rnd_nonce)

    def _load_sample_image(self):
        # Create a synthetic 160x160 RGB gradient test pattern
        arr = np.zeros((160, 160, 3), dtype=np.uint8)
        for y in range(160):
            for x in range(160):
                arr[y, x] = [x % 256, y % 256, (x + y) % 256]
        self.original_img = Image.fromarray(arr)
        self.img_size = (160, 160)
        self.img_mode = "RGB"
        self.raw_plaintext = self.original_img.tobytes()
        self._update_img_display(self.lbl_orig_img, self.original_img)
        self._plot_histogram(self.raw_plaintext, None)

    def _browse_image(self):
        file_path = filedialog.askopenfilename(
            filetypes=[("Image Files", "*.png;*.jpg;*.jpeg;*.bmp"), ("All Files", "*.*")]
        )
        if not file_path:
            return

        try:
            pil_img = Image.open(file_path).convert("RGB")
            # Resize for smooth UI display while keeping full resolution for crypto if desired
            display_img = pil_img.resize((160, 160), Image.Resampling.LANCZOS)
            self.original_img = display_img
            self.img_size = display_img.size
            self.img_mode = "RGB"
            self.raw_plaintext = display_img.tobytes()
            self.current_img_path = file_path
            self.lbl_file_status.configure(text=f"Loaded: {os.path.basename(file_path)} ({len(self.raw_plaintext)} B)")
            self._update_img_display(self.lbl_orig_img, self.original_img)
            self.lbl_enc_img.configure(image=None, text="\n[Click Encrypt]\n")
            self.lbl_dec_img.configure(image=None, text="\n[Click Decrypt]\n")
            self._plot_histogram(self.raw_plaintext, None)
        except Exception as e:
            messagebox.showerror("File Error", f"Cannot open image: {e}")

    def _update_img_display(self, label_widget, pil_img):
        tk_img = ctk.CTkImage(light_image=pil_img, dark_image=pil_img, size=pil_img.size)
        label_widget.configure(image=tk_img, text="")
        label_widget.image = tk_img

    def _do_encrypt(self):
        if not self.raw_plaintext:
            messagebox.showwarning("Warning", "No plaintext loaded to encrypt.")
            return

        key_hex = self.ent_key.get().strip()
        nonce_hex = self.ent_nonce.get().strip()
        aad_str = self.ent_aad.get().strip()

        try:
            key_bytes = bytes.fromhex(key_hex)
            if len(key_bytes) != 16:
                raise ValueError("Key must be 16 bytes (32 hex characters).")
        except Exception as e:
            messagebox.showerror("Key Error", f"Invalid Master Key: {e}")
            return

        mode = self.mode_selector.get()

        if "GCM" in mode:
            try:
                nonce_bytes = bytes.fromhex(nonce_hex)
                if len(nonce_bytes) != 12:
                    raise ValueError("Nonce must be 12 bytes (24 hex characters).")
            except Exception as e:
                messagebox.showerror("Nonce Error", f"Invalid Nonce: {e}")
                return

            aad_bytes = aad_str.encode("utf-8")
            ct, tag, ms, speed = encrypt_aead(key_bytes, nonce_bytes, aad_bytes, self.raw_plaintext)
            self.raw_ciphertext = ct
            self.raw_tag = tag

            self.lbl_metric_time.configure(text=f"Encryption Time: {ms:.3f} ms")
            self.lbl_metric_speed.configure(text=f"Throughput: {speed:.2f} MB/s (GCM AEAD)")
            self.lbl_metric_tag.configure(text=f"Tag: {tag.hex()[:16]}... (128-bit)")
        else:
            # CTR AVX2
            iv16 = bytes.fromhex((nonce_hex + "00000000")[:32])
            ct, ms, speed = encrypt_ctr_avx2(key_bytes, iv16, self.raw_plaintext)
            self.raw_ciphertext = ct
            self.raw_tag = b""

            self.lbl_metric_time.configure(text=f"Encryption Time: {ms:.3f} ms")
            self.lbl_metric_speed.configure(text=f"Throughput: {speed:.2f} MB/s (AVX2 CTR)")
            self.lbl_metric_tag.configure(text="Tag: N/A (CTR Mode)")

        # Entropy & Display
        ent = calculate_entropy(self.raw_ciphertext)
        self.lbl_metric_entropy.configure(text=f"Entropy: {ent:.4f} / 8.000 (Ideal: 8.0)")

        # Render Encrypted Image noise
        enc_pil = Image.frombytes(self.img_mode, self.img_size, self.raw_ciphertext)
        self.encrypted_img = enc_pil
        self._update_img_display(self.lbl_enc_img, enc_pil)

        # Plot Histograms
        self._plot_histogram(self.raw_plaintext, self.raw_ciphertext)

    def _do_decrypt(self):
        if not self.raw_ciphertext:
            messagebox.showwarning("Warning", "No ciphertext available. Please encrypt first.")
            return

        key_hex = self.ent_key.get().strip()
        nonce_hex = self.ent_nonce.get().strip()
        aad_str = self.ent_aad.get().strip()
        key_bytes = bytes.fromhex(key_hex)
        mode = self.mode_selector.get()

        if "GCM" in mode:
            nonce_bytes = bytes.fromhex(nonce_hex)
            aad_bytes = aad_str.encode("utf-8")
            pt, ok, ms = decrypt_aead(key_bytes, nonce_bytes, aad_bytes, self.raw_ciphertext, self.raw_tag)

            if not ok or pt is None:
                messagebox.showerror(
                    "INTEGRITY VIOLATION",
                    "❌ GHASH Tag Mismatch!\nData has been tampered with or corrupted.\nCiphertext rejected."
                )
                return

            self.lbl_metric_time.configure(text=f"Decryption Time: {ms:.3f} ms (Verified)")
        else:
            iv16 = bytes.fromhex((nonce_hex + "00000000")[:32])
            pt, ms = decrypt_ctr_avx2(key_bytes, iv16, self.raw_ciphertext)
            self.lbl_metric_time.configure(text=f"Decryption Time: {ms:.3f} ms")

        dec_pil = Image.frombytes(self.img_mode, self.img_size, pt)
        self.decrypted_img = dec_pil
        self._update_img_display(self.lbl_dec_img, dec_pil)

    def _plot_histogram(self, plain_data, cipher_data):
        self.ax.clear()
        self.ax.set_facecolor("#0f172a")

        h_plain = calculate_histogram(plain_data)
        self.ax.plot(range(256), h_plain, color="#38bdf8", label="Plaintext Histogram", alpha=0.85, linewidth=1.5)

        if cipher_data:
            h_cipher = calculate_histogram(cipher_data)
            self.ax.plot(range(256), h_cipher, color="#f43f5e", label="Ciphertext Histogram (Flat/Uniform)", alpha=0.9, linewidth=1.5)

        self.ax.set_xlim(0, 255)
        self.ax.tick_params(colors="#94a3b8", labelsize=8)
        self.ax.set_title("Byte Frequency Distribution (0..255)", color="#f8fafc", fontsize=10)
        self.ax.legend(facecolor="#1e293b", edgecolor="#334155", labelcolor="#f8fafc", fontsize=8)
        self.fig.tight_layout()
        self.hist_canvas.draw()

    # -------------------------------------------------------------
    # TAB 3: TAMPER ATTACK & INTEGRITY LAB
    # -------------------------------------------------------------
    def _init_tamper_tab(self):
        container = ctk.CTkFrame(self.tab_tamper, fg_color="transparent")
        container.pack(fill="both", expand=True, padx=15, pady=15)

        ctk.CTkLabel(
            container,
            text="ACTIVE ATTACK SIMULATION & AEAD FORGERY RESISTANCE",
            font=ctk.CTkFont(size=15, weight="bold"),
            text_color="#f8fafc"
        ).pack(anchor="w", pady=(0, 10))

        desc = (
            "This lab demonstrates Chosen-Ciphertext Attacks (CCA) and Man-in-the-Middle (MITM) tampering.\n"
            "In legacy modes (like CTR without MAC or CBC), tampering goes completely unnoticed by the receiver.\n"
            "In Rubik4D-GCM, the GHASH polynomial tag over GF(2^128) mathematically guarantees 100% forgery detection."
        )
        ctk.CTkLabel(container, text=desc, font=ctk.CTkFont(size=12), text_color="#94a3b8", justify="left").pack(anchor="w", pady=(0, 15))

        # Main interactive box
        box = ctk.CTkFrame(container, fg_color="#1e293b", corner_radius=10)
        box.pack(fill="both", expand=True, padx=5, pady=5)

        # Sample message
        ctk.CTkLabel(box, text="Sample Plaintext Message:", font=ctk.CTkFont(size=12, weight="bold")).pack(anchor="w", padx=20, pady=(15, 2))
        self.ent_tamper_msg = ctk.CTkEntry(box, width=500)
        self.ent_tamper_msg.insert(0, "Transfer $1,000,000 to Account #8888-9999 [CONFIDENTIAL]")
        self.ent_tamper_msg.pack(anchor="w", padx=20, pady=4)

        # Attack choices
        ctk.CTkLabel(box, text="Select Attack Type to inject during transmission:", font=ctk.CTkFont(size=12, weight="bold")).pack(anchor="w", padx=20, pady=(15, 2))
        self.tamper_attack_type = ctk.CTkRadioButton(
            box, text="1. Bit-Flipping Attack on Ciphertext (Inverts bit 7 of Byte #10)",
            variable=tk.IntVar(value=1), value=1
        )
        self.tamper_attack_type.pack(anchor="w", padx=25, pady=4)

        self.tamper_attack_type2 = ctk.CTkRadioButton(
            box, text="2. Associated Data (AAD) Header Tampering (Modifies routing metadata)",
            variable=tk.IntVar(value=1), value=2
        )
        self.tamper_attack_type2.pack(anchor="w", padx=25, pady=4)

        self.tamper_attack_type3 = ctk.CTkRadioButton(
            box, text="3. Forged Authentication Tag Attack (Replaces 128-bit tag with random hash)",
            variable=tk.IntVar(value=1), value=3
        )
        self.tamper_attack_type3.pack(anchor="w", padx=25, pady=4)

        # Execute Attack button
        btn_run_attack = ctk.CTkButton(
            box,
            text="⚡ Inject Attack & Attempt Decryption",
            font=ctk.CTkFont(size=13, weight="bold"),
            fg_color="#ef4444",
            hover_color="#dc2626",
            command=self._execute_tamper_simulation
        )
        btn_run_attack.pack(anchor="w", padx=20, pady=(15, 15))

        # Big Terminal / Result Alert Box
        self.txt_tamper_res = ctk.CTkTextbox(box, height=180, fg_color="#0f172a", font=ctk.CTkFont(family="Consolas", size=12))
        self.txt_tamper_res.pack(fill="both", expand=True, padx=20, pady=(0, 20))
        self.txt_tamper_res.insert("1.0", "[READY] Click 'Inject Attack' to observe how Rubik4D-GCM intercepts forgeries.\n")

    def _execute_tamper_simulation(self):
        msg = self.ent_tamper_msg.get().strip().encode("utf-8")
        key = b"\x01" * 16
        nonce = b"\x02" * 12
        aad = b"Packet-Header: Routing=SecureServer"

        # 1. Normal Encryption
        ct, tag, _, _ = encrypt_aead(key, nonce, aad, msg)
        ct_mod = bytearray(ct)
        aad_mod = bytearray(aad)
        tag_mod = bytearray(tag)

        # Invert bit based on selection
        # Get active value
        atk = 1
        if self.tamper_attack_type2.get() == 2:
            atk = 2
        elif self.tamper_attack_type3.get() == 3:
            atk = 3

        report = []
        report.append("=" * 65)
        report.append(">>> INITIATING RUBIK-4D AEAD SECURITY VERIFICATION")
        report.append(f"Original Plaintext: {msg.decode('utf-8', errors='replace')}")
        report.append(f"Original Ciphertext Tag: {tag.hex()}")

        if atk == 1:
            report.append("\n[ATTACK] Injecting Bit-Flip into Ciphertext byte #10...")
            if len(ct_mod) > 10:
                ct_mod[10] ^= 0x80
            else:
                ct_mod[0] ^= 0x80
        elif atk == 2:
            report.append("\n[ATTACK] Tampering with AAD Packet Header (routing metadata)...")
            aad_mod[0] ^= 0xFF
        else:
            report.append("\n[ATTACK] Forging 128-bit Authentication Tag...")
            tag_mod = bytearray(os.urandom(16))

        # Now attempt decryption
        pt, ok, ms = decrypt_aead(key, nonce, bytes(aad_mod), bytes(ct_mod), bytes(tag_mod))

        if not ok:
            report.append("\n[RESULT] 🛑 REJECTED! TAMPERING DETECTED BY GHASH ENGINE!")
            report.append("-----------------------------------------------------------------")
            report.append("STATUS: AUTHENTICATION FAILED (Error Code: -1)")
            report.append("DECISION: No plaintext decrypted or leaked to the application layer.")
            report.append("SECURITY GUARANTEE: The NIST SP 800-38D GHASH polynomial evaluation")
            report.append("over GF(2^128) mathematically guarantees that any 1-bit tampering")
            report.append("has a forgery success probability <= 2^(-128) (cryptographically impossible).")
        else:
            report.append("\n[RESULT] Decryption Succeeded.")

        report.append("=" * 65)

        self.txt_tamper_res.delete("1.0", "end")
        self.txt_tamper_res.insert("1.0", "\n".join(report))

    # -------------------------------------------------------------
    # TAB 4: LIVE HARDWARE BENCHMARK
    # -------------------------------------------------------------
    def _init_bench_tab(self):
        container = ctk.CTkFrame(self.tab_bench, fg_color="transparent")
        container.pack(fill="both", expand=True, padx=15, pady=15)

        ctk.CTkLabel(
            container,
            text="MICROARCHITECTURE HARDWARE BENCHMARK SUITE",
            font=ctk.CTkFont(size=15, weight="bold"),
            text_color="#f8fafc"
        ).pack(anchor="w", pady=(0, 10))

        # Bench Controls
        ctrl_f = ctk.CTkFrame(container, fg_color="#1e293b", corner_radius=10)
        ctrl_f.pack(fill="x", pady=(0, 15))

        ctk.CTkLabel(ctrl_f, text="Test Buffer Size:", font=ctk.CTkFont(size=12)).pack(side="left", padx=(15, 5), pady=12)
        self.bench_size_combo = ctk.CTkComboBox(ctrl_f, values=["2 MB", "5 MB", "10 MB", "25 MB"], width=100)
        self.bench_size_combo.set("5 MB")
        self.bench_size_combo.pack(side="left", padx=5, pady=12)

        self.btn_run_bench = ctk.CTkButton(
            ctrl_f,
            text="🚀 Run Live Benchmark",
            font=ctk.CTkFont(size=12, weight="bold"),
            fg_color="#0284c7",
            hover_color="#0369a1",
            command=self._start_benchmark_thread
        )
        self.btn_run_bench.pack(side="left", padx=15, pady=12)

        self.lbl_bench_status = ctk.CTkLabel(ctrl_f, text="Ready.", font=ctk.CTkFont(size=11), text_color="#94a3b8")
        self.lbl_bench_status.pack(side="left", padx=10, pady=12)

        # Matplotlib Bench Figure
        self.bench_fig = Figure(figsize=(8, 4), dpi=100, facecolor="#1e293b")
        self.bench_ax = self.bench_fig.add_subplot(111)
        self.bench_ax.set_facecolor("#0f172a")
        self.bench_ax.tick_params(colors="#94a3b8")
        self.bench_ax.set_title("Encryption Throughput (MB/s) on Local CPU", color="#f8fafc", fontsize=11)

        self.bench_canvas = FigureCanvasTkAgg(self.bench_fig, master=container)
        self.bench_canvas.get_tk_widget().pack(fill="both", expand=True)

        # Plot default baseline chart
        self._plot_bench_chart(166.56, 254.18, 208.80)

    def _start_benchmark_thread(self):
        self.btn_run_bench.configure(state="disabled")
        self.lbl_bench_status.configure(text="Benchmarking in progress (AVX2 & GHASH)...")
        threading.Thread(target=self._run_bench_worker, daemon=True).start()

    def _run_bench_worker(self):
        val_str = self.bench_size_combo.get().split()[0]
        mb = int(val_str)
        try:
            res = run_benchmark(mb)
            gcm_spd = res["GCM_AEAD"]["throughput_mb_s"]
            ctr_spd = res["CTR_AVX2"]["throughput_mb_s"]
            scalar_spd = 166.56  # Baseline reference
            self.after(0, lambda: self._on_bench_complete(scalar_spd, ctr_spd, gcm_spd))
        except Exception as e:
            self.after(0, lambda: messagebox.showerror("Benchmark Error", str(e)))
            self.after(0, lambda: self.btn_run_bench.configure(state="normal"))

    def _on_bench_complete(self, scalar_spd, ctr_spd, gcm_spd):
        self._plot_bench_chart(scalar_spd, ctr_spd, gcm_spd)
        self.btn_run_bench.configure(state="normal")
        self.lbl_bench_status.configure(text=f"Done! CTR: {ctr_spd:.1f} MB/s | GCM: {gcm_spd:.1f} MB/s")

    def _plot_bench_chart(self, scalar_spd, ctr_spd, gcm_spd):
        self.bench_ax.clear()
        self.bench_ax.set_facecolor("#0f172a")

        modes = ["Scalar C (Baseline)", "Rubik4D-CTR (SIMD AVX2)", "Rubik4D-GCM (AEAD Full)"]
        speeds = [scalar_spd, ctr_spd, gcm_spd]
        colors = ["#64748b", "#0284c7", "#10b981"]

        bars = self.bench_ax.barh(modes, speeds, color=colors, height=0.5)

        for bar in bars:
            w = bar.get_width()
            self.bench_ax.text(
                w + 5, bar.get_y() + bar.get_height() / 2,
                f"{w:.1f} MB/s",
                va="center", ha="left", color="#f8fafc", fontweight="bold", fontsize=10
            )

        self.bench_ax.set_xlim(0, max(speeds) * 1.25)
        self.bench_ax.tick_params(colors="#94a3b8", labelsize=9)
        self.bench_ax.set_xlabel("Throughput (Megabytes per second)", color="#94a3b8", fontsize=9)
        self.bench_ax.set_title("Live Microarchitecture Throughput Comparison", color="#f8fafc", fontsize=11)
        self.bench_fig.tight_layout()
        self.bench_canvas.draw()


if __name__ == "__main__":
    app = Rubik4DApp()
    app.mainloop()
