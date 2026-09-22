#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Master Test Runner for Rubik-4D Cryptanalysis Suite
Compiles all tests into tests/bin/, executes tests, and saves all reports into reports/
"""

import os
import sys
import subprocess

def run_cmd(cmd, cwd=None):
    print(f"\n[EXEC] {cmd}")
    res = subprocess.run(cmd, shell=True, cwd=cwd)
    if res.returncode != 0:
        print(f"[ERROR] Command failed with code {res.returncode}: {cmd}")
        sys.exit(res.returncode)

def main():
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    tests_dir = os.path.join(root_dir, "tests")
    bin_dir = os.path.join(tests_dir, "bin")
    reports_dir = os.path.join(root_dir, "reports")

    os.makedirs(bin_dir, exist_ok=True)
    os.makedirs(reports_dir, exist_ok=True)

    print("====================================================================")
    print(" 🧊 RUBIK-4D CRYPTANALYSIS SUITE - AUTOMATED BUILD & TEST RUNNER")
    print("====================================================================")

    # 1. Compile binaries into tests/bin/
    print("\n--- [Step 1/4] Compiling Core Test Binaries into tests/bin/ ---")
    run_cmd(f"gcc -O3 -shared src/rubik4d.c -Iinclude -o tests/bin/librubik4d.dll", cwd=root_dir)
    run_cmd(f"gcc -O3 tests/test_key_schedule.c src/rubik4d.c -Iinclude -o tests/bin/test_key_schedule.exe", cwd=root_dir)
    run_cmd(f"gcc -O3 tests/test_sensitivity.c src/rubik4d.c -Iinclude -o tests/bin/test_sensitivity.exe", cwd=root_dir)
    run_cmd(f"gcc -O3 tests/test_benchmark.c src/rubik4d.c src/aes128.c src/speck128.c src/simon128.c src/chacha20.c -Iinclude -o tests/bin/test_benchmark.exe", cwd=root_dir)

    # 2. Run Test 1: Key Schedule SAC & Weak Key Analysis
    print("\n--- [Step 2/4] Running Key Schedule Security & SAC Analysis ---")
    raw_sac = os.path.join(reports_dir, "raw_key_schedule_sac.txt")
    run_cmd(f'"{os.path.join(bin_dir, "test_key_schedule.exe")}" > "{raw_sac}"', cwd=root_dir)

    # 3. Run Test 2: Plaintext Avalanche & Key Sensitivity
    print("\n--- [Step 3/4] Running Plaintext & Key Sensitivity Evaluation ---")
    raw_sens = os.path.join(reports_dir, "raw_sensitivity.txt")
    run_cmd(f'"{os.path.join(bin_dir, "test_sensitivity.exe")}" > "{raw_sens}"', cwd=root_dir)

    # 4. Run Test 3: Image Cryptanalysis
    print("\n--- [Step 4/4] Running Image Cryptanalysis on Lena & Baboon ---")
    raw_img = os.path.join(reports_dir, "raw_image_cryptanalysis.txt")
    run_cmd(f'python tests/test_image_analysis.py > "{raw_img}"', cwd=root_dir)

    # 5. Generate LaTeX Tables and Markdown Report
    print("\n--- Generating Consolidated Reports & LaTeX Tables into reports/ ---")
    run_cmd(f"python tests/generate_report.py", cwd=root_dir)

    print("\n====================================================================")
    print(" [SUCCESS] All cryptanalysis tests completed and saved to reports/")
    print(f" Directory: {reports_dir}")
    print("====================================================================\n")

if __name__ == "__main__":
    main()
