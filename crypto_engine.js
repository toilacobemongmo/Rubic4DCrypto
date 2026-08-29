const Rubik4DCrypto = (() => {
    const SBOX = [
        0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
        0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
        0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
        0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
        0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
        0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
        0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
        0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
        0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
        0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
        0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
        0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
        0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
        0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
        0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
        0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
    ];

    const INV_SBOX = new Uint8Array(256);
    SBOX.forEach((val, idx) => INV_SBOX[val] = idx);

    const P_FWD = [3, 1, 11, 9, 7, 5, 15, 13, 2, 0, 10, 8, 6, 4, 14, 12];

    function rotate4D(arr16, invert = false) {
        const out = new Uint8Array(16);
        if (!invert) {
            for (let i = 0; i < 16; i++) out[i] = arr16[P_FWD[i]];
        } else {
            for (let i = 0; i < 16; i++) out[P_FWD[i]] = arr16[i];
        }
        return out;
    }

    // Tầng khuếch tán Modulo-Cộng & XOR ARX trên 4 chiều
    function mixDiffusion(state) {
        const out = new Uint8Array(state);
        for (let i = 0; i < 16; i++) {
            const nextIdx = (i + 1) % 16;
            out[nextIdx] = (out[nextIdx] ^ (out[i] + 0x5a)) & 0xff;
        }
        return out;
    }

    // Nghịch đảo chính xác tầng khuếch tán ARX
    function invMixDiffusion(state) {
        const out = new Uint8Array(state);
        for (let i = 15; i >= 0; i--) {
            const nextIdx = (i + 1) % 16;
            out[nextIdx] = (out[nextIdx] ^ (out[i] + 0x5a)) & 0xff;
        }
        return out;
    }

    function generateRoundKeys(password, rounds = 12) {
        let seed = 0;
        const enc = new TextEncoder().encode(password);
        for (let i = 0; i < enc.length; i++) {
            seed = (Math.imul(seed, 31) + enc[i]) >>> 0;
        }

        const roundKeys = [];
        for (let r = 0; r <= rounds; r++) {
            let currentSeed = (seed + Math.imul(r, 0x9e3779b9)) >>> 0;
            const rk = new Uint8Array(16);
            for (let i = 0; i < 16; i++) {
                currentSeed = (Math.imul(currentSeed, 1664525) + 1013904223) >>> 0;
                rk[i] = (currentSeed >>> 24) & 0xFF;
            }
            roundKeys.push(rk);
        }
        return roundKeys;
    }

    function encryptBlock(block16, roundKeys, rounds = 12) {
        let state = new Uint8Array(16);
        for (let i = 0; i < 16; i++) state[i] = block16[i] ^ roundKeys[0][i];

        for (let r = 1; r <= rounds; r++) {
            for (let i = 0; i < 16; i++) state[i] = SBOX[state[i]];
            state = rotate4D(state, false);
            state = mixDiffusion(state); // Khuếch tán bit liên byte
            for (let i = 0; i < 16; i++) state[i] ^= roundKeys[r][i];
        }
        return state;
    }

    function decryptBlock(block16, roundKeys, rounds = 12) {
        let state = new Uint8Array(block16);
        for (let r = rounds; r >= 1; r--) {
            for (let i = 0; i < 16; i++) state[i] ^= roundKeys[r][i];
            state = invMixDiffusion(state); // Nghịch đảo khuếch tán
            state = rotate4D(state, true);
            for (let i = 0; i < 16; i++) state[i] = INV_SBOX[state[i]];
        }
        for (let i = 0; i < 16; i++) state[i] ^= roundKeys[0][i];
        return state;
    }

    function encrypt(text, password, rounds = 12) {
        const rawBytes = new TextEncoder().encode(text);
        const rkeys = generateRoundKeys(password, rounds);
        const padLen = 16 - (rawBytes.length % 16);
        const padded = new Uint8Array(rawBytes.length + padLen);
        padded.set(rawBytes);
        for (let i = rawBytes.length; i < padded.length; i++) padded[i] = padLen;

        const hexOut = [];
        for (let i = 0; i < padded.length; i += 16) {
            const block = padded.slice(i, i + 16);
            const enc = encryptBlock(block, rkeys, rounds);
            for (let j = 0; j < 16; j++) {
                hexOut.push(enc[j].toString(16).padStart(2, '0'));
            }
        }
        return hexOut.join('');
    }

    function decrypt(hex, password, rounds = 12) {
        const cleanHex = hex.trim().replace(/\s+/g, '');
        if (cleanHex.length === 0 || cleanHex.length % 32 !== 0) {
            throw new Error("Bản mã Hex không hợp lệ!");
        }

        const rkeys = generateRoundKeys(password, rounds);
        const byteLen = cleanHex.length / 2;
        const bytes = new Uint8Array(byteLen);
        for (let i = 0; i < byteLen; i++) {
            bytes[i] = parseInt(cleanHex.substring(i * 2, i * 2 + 2), 16);
        }

        const decrypted = new Uint8Array(byteLen);
        for (let i = 0; i < byteLen; i += 16) {
            const block = bytes.slice(i, i + 16);
            const dec = decryptBlock(block, rkeys, rounds);
            decrypted.set(dec, i);
        }

        const padLen = decrypted[decrypted.length - 1];
        if (padLen < 1 || padLen > 16) throw new Error("Khóa sai hoặc bản mã hỏng!");
        for (let i = decrypted.length - padLen; i < decrypted.length; i++) {
            if (decrypted[i] !== padLen) throw new Error("Padding không hợp lệ!");
        }

        return new TextDecoder().decode(decrypted.slice(0, decrypted.length - padLen));
    }

    return { encrypt, decrypt, encryptBlock, generateRoundKeys };
})();