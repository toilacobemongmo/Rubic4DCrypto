"""
Rubik-4D Python Engine Wrapper
Provides high-performance ctypes bindings to the native C SIMD/AEAD rubik4d.dll
"""

import os
import sys
import time
import math
import ctypes
from typing import Tuple, Optional

# Locate DLL
_BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_DLL_PATH = os.path.join(_BASE_DIR, "lib", "rubik4d.dll")

if not os.path.exists(_DLL_PATH):
    raise FileNotFoundError(f"rubik4d.dll not found at {_DLL_PATH}. Please compile it first.")

_lib = ctypes.CDLL(_DLL_PATH)

# Initialize tables
_lib.rubik4d_init_tables.argtypes = []
_lib.rubik4d_init_tables.restype = None
_lib.rubik4d_init_tables()

# CPUID checks
_lib.rubik4d_has_ssse3.argtypes = []
_lib.rubik4d_has_ssse3.restype = ctypes.c_int
_lib.rubik4d_has_avx2.argtypes = []
_lib.rubik4d_has_avx2.restype = ctypes.c_int
_lib.rubik4d_has_pclmul.argtypes = []
_lib.rubik4d_has_pclmul.restype = ctypes.c_int

HAS_SSSE3 = bool(_lib.rubik4d_has_ssse3())
HAS_AVX2 = bool(_lib.rubik4d_has_avx2())
HAS_PCLMUL = bool(_lib.rubik4d_has_pclmul())

# AEAD GCM API
_lib.rubik4d_aead_encrypt.argtypes = [
    ctypes.c_char_p, ctypes.c_size_t,  # key, key_len
    ctypes.c_char_p, ctypes.c_size_t,  # iv, iv_len
    ctypes.c_char_p, ctypes.c_size_t,  # aad, aad_len
    ctypes.c_char_p, ctypes.c_size_t,  # plain, plain_len
    ctypes.c_char_p, ctypes.c_char_p   # cipher, tag[16]
]
_lib.rubik4d_aead_encrypt.restype = ctypes.c_int

_lib.rubik4d_aead_decrypt.argtypes = [
    ctypes.c_char_p, ctypes.c_size_t,  # key, key_len
    ctypes.c_char_p, ctypes.c_size_t,  # iv, iv_len
    ctypes.c_char_p, ctypes.c_size_t,  # aad, aad_len
    ctypes.c_char_p, ctypes.c_size_t,  # cipher, cipher_len
    ctypes.c_char_p, ctypes.c_char_p   # tag[16], plain
]
_lib.rubik4d_aead_decrypt.restype = ctypes.c_int

# CBC Fallback API
_lib.rubik4d_encrypt.argtypes = [
    ctypes.c_char_p, ctypes.c_size_t,  # in, in_len
    ctypes.c_char_p,                    # out
    ctypes.c_char_p, ctypes.c_size_t,  # key, key_len
    ctypes.c_char_p                     # iv[16]
]
_lib.rubik4d_encrypt.restype = ctypes.c_size_t

_lib.rubik4d_decrypt.argtypes = [
    ctypes.c_char_p, ctypes.c_size_t,  # in, in_len
    ctypes.c_char_p,                    # out
    ctypes.c_char_p, ctypes.c_size_t,  # key, key_len
    ctypes.c_char_p                     # iv[16]
]
_lib.rubik4d_decrypt.restype = ctypes.c_size_t

# Context definition for AVX2 CTR
class Rubik4DCtx(ctypes.Structure):
    _fields_ = [
        ("round_keys", (ctypes.c_uint8 * 16) * 9),
        ("perm_lut", ctypes.c_void_p * 9)
    ]

_lib.rubik4d_key_setup.argtypes = [ctypes.POINTER(Rubik4DCtx), ctypes.c_char_p, ctypes.c_size_t]
_lib.rubik4d_key_setup.restype = None

_lib.rubik4d_ctr_encrypt_avx2.argtypes = [
    ctypes.POINTER(Rubik4DCtx),
    ctypes.c_char_p, ctypes.c_char_p, ctypes.c_size_t,
    ctypes.c_char_p
]
_lib.rubik4d_ctr_encrypt_avx2.restype = None


def calculate_entropy(data: bytes) -> float:
    """Calculates Shannon entropy in bits per byte (0.0 to 8.0)."""
    if not data:
        return 0.0
    freq = [0] * 256
    for b in data:
        freq[b] += 1
    total = len(data)
    ent = 0.0
    for count in freq:
        if count > 0:
            p = count / total
            ent -= p * math.log2(p)
    return ent


def calculate_histogram(data: bytes) -> list:
    """Calculates frequency distribution of byte values [0..255]."""
    hist = [0] * 256
    for b in data:
        hist[b] += 1
    return hist


def encrypt_aead(key: bytes, nonce: bytes, aad: bytes, plaintext: bytes) -> Tuple[bytes, bytes, float, float]:
    """
    Encrypts plaintext using Rubik4D-GCM AEAD (NIST SP 800-38D).
    Returns (ciphertext, tag, elapsed_ms, throughput_mb_s).
    """
    if len(key) != 16:
        raise ValueError("Key must be 16 bytes (128-bit).")
    if len(nonce) != 12:
        raise ValueError("Nonce must be 12 bytes (96-bit).")
    
    ct_buf = ctypes.create_string_buffer(len(plaintext))
    tag_buf = ctypes.create_string_buffer(16)
    
    t0 = time.perf_counter()
    res = _lib.rubik4d_aead_encrypt(key, len(key), nonce, len(nonce), aad, len(aad), plaintext, len(plaintext), ct_buf, tag_buf)
    t1 = time.perf_counter()
    
    if res != 0:
        raise RuntimeError("Rubik4D-GCM encryption failed.")
    
    elapsed_ms = (t1 - t0) * 1000.0
    mb_s = (len(plaintext) / (1024 * 1024)) / (t1 - t0) if (t1 - t0) > 0 else 0.0
    return ct_buf.raw, tag_buf.raw, elapsed_ms, mb_s


def decrypt_aead(key: bytes, nonce: bytes, aad: bytes, ciphertext: bytes, tag: bytes) -> Tuple[Optional[bytes], bool, float]:
    """
    Decrypts ciphertext and verifies GCM tag in constant-time.
    Returns (plaintext_or_None, is_authentic, elapsed_ms).
    """
    if len(key) != 16:
        raise ValueError("Key must be 16 bytes (128-bit).")
    if len(nonce) != 12:
        raise ValueError("Nonce must be 12 bytes (96-bit).")
    if len(tag) != 16:
        raise ValueError("Tag must be 16 bytes (128-bit).")
    
    pt_buf = ctypes.create_string_buffer(len(ciphertext))
    
    t0 = time.perf_counter()
    res = _lib.rubik4d_aead_decrypt(key, len(key), nonce, len(nonce), aad, len(aad), ciphertext, len(ciphertext), tag, pt_buf)
    t1 = time.perf_counter()
    
    elapsed_ms = (t1 - t0) * 1000.0
    if res == 0:
        return pt_buf.raw, True, elapsed_ms
    else:
        return None, False, elapsed_ms


def encrypt_ctr_avx2(key: bytes, iv: bytes, plaintext: bytes) -> Tuple[bytes, float, float]:
    """
    Encrypts plaintext using Rubik4D-CTR SIMD AVX2 parallel engine.
    Returns (ciphertext, elapsed_ms, throughput_mb_s).
    """
    if len(key) != 16:
        raise ValueError("Key must be 16 bytes.")
    if len(iv) != 16:
        raise ValueError("IV must be 16 bytes.")
    
    ctx = Rubik4DCtx()
    _lib.rubik4d_key_setup(ctypes.byref(ctx), key, len(key))
    
    out_buf = ctypes.create_string_buffer(len(plaintext))
    
    t0 = time.perf_counter()
    _lib.rubik4d_ctr_encrypt_avx2(ctypes.byref(ctx), plaintext, out_buf, len(plaintext), iv)
    t1 = time.perf_counter()
    
    elapsed_ms = (t1 - t0) * 1000.0
    mb_s = (len(plaintext) / (1024 * 1024)) / (t1 - t0) if (t1 - t0) > 0 else 0.0
    return out_buf.raw, elapsed_ms, mb_s


def decrypt_ctr_avx2(key: bytes, iv: bytes, ciphertext: bytes) -> Tuple[bytes, float]:
    """CTR decryption is symmetric to CTR encryption."""
    pt, elapsed_ms, _ = encrypt_ctr_avx2(key, iv, ciphertext)
    return pt, elapsed_ms


def run_benchmark(size_mb: int = 10) -> dict:
    """Runs a live performance comparison benchmark."""
    raw_data = os.urandom(size_mb * 1024 * 1024)
    key = os.urandom(16)
    iv12 = os.urandom(12)
    iv16 = os.urandom(16)
    aad = b"Network-Packet-Auth-Header"
    
    results = {}
    
    # 1. Rubik4D-GCM AEAD
    _, _, ms_gcm, mb_s_gcm = encrypt_aead(key, iv12, aad, raw_data)
    results['GCM_AEAD'] = {
        'throughput_mb_s': round(mb_s_gcm, 2),
        'latency_ms': round(ms_gcm, 2)
    }
    
    # 2. Rubik4D-CTR AVX2
    _, ms_ctr, mb_s_ctr = encrypt_ctr_avx2(key, iv16, raw_data)
    results['CTR_AVX2'] = {
        'throughput_mb_s': round(mb_s_ctr, 2),
        'latency_ms': round(ms_ctr, 2)
    }
    
    return results
