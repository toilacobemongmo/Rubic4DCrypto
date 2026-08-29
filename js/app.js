/**
 * Module điều hướng giao diện UI & Event Binding - CỐ ĐỊNH CHUẨN 12 VÒNG
 */
document.addEventListener('DOMContentLoaded', () => {
    // Khởi tạo Lucide Icons an toàn
    if (typeof lucide !== 'undefined' && lucide.createIcons) {
        lucide.createIcons();
    }

    const masterKeyInput = document.getElementById('masterKey');
    const toggleKeyVis = document.getElementById('toggleKeyVis');
    const plainInput = document.getElementById('plainInput');
    const cipherInput = document.getElementById('cipherInput');
    
    // Khối output giải mã
    const recoveredPlainInput = document.getElementById('recoveredPlainInput') || document.getElementById('plainOutput');
    
    const encryptBtn = document.getElementById('encryptBtn');
    const decryptBtn = document.getElementById('decryptBtn');
    const clearPlainBtn = document.getElementById('clearPlainBtn');
    const copyCipherBtn = document.getElementById('copyCipherBtn');
    const runAvalancheBtn = document.getElementById('runAvalancheBtn');

    // Khởi tạo Key từ Storage nếu có
    if (masterKeyInput && typeof AppStorage !== 'undefined') {
        masterKeyInput.value = AppStorage.loadKey() || 'PTIT_ATTT_B24DCAT088';
        masterKeyInput.addEventListener('input', (e) => {
            AppStorage.saveKey(e.target.value);
            showToast('Mật khẩu đã được lưu tự động!');
        });
    }
    
    renderTesseractGrid(new Uint8Array(16));

    if (toggleKeyVis && masterKeyInput) {
        toggleKeyVis.addEventListener('click', () => {
            masterKeyInput.type = masterKeyInput.type === 'password' ? 'text' : 'password';
        });
    }

    // Chuyển Tabs giao diện
    ['crypto', 'avalanche', 'tesseract'].forEach(tab => {
        const tabEl = document.getElementById(`tab-${tab}`);
        if (!tabEl) return;
        tabEl.addEventListener('click', () => {
            document.querySelectorAll('.tab-view').forEach(v => v.classList.add('hidden'));
            document.querySelectorAll('.tab-btn').forEach(b => {
                b.className = 'tab-btn px-4 py-2 rounded-lg text-slate-400 hover:text-white flex items-center gap-2';
            });
            const targetView = document.getElementById(`view-${tab}`);
            if (targetView) targetView.classList.remove('hidden');
            tabEl.className = 'tab-btn px-4 py-2 rounded-lg bg-cyan-500 text-black font-semibold flex items-center gap-2';
            if (typeof lucide !== 'undefined' && lucide.createIcons) lucide.createIcons();
        });
    });

    // 1. SỰ KIỆN MÃ HÓA (CỐ ĐỊNH 12 VÒNG)
    if (encryptBtn) {
        encryptBtn.addEventListener('click', () => {
            const key = getElementData('masterKey') || 'PTIT_ATTT_B24DCAT088';
            const text = getElementData('plainInput');
            
            if (!key) return showToast('Vui lòng nhập Key!');
            if (!text) return showToast('Vui lòng nhập Plaintext!');

            // ÉP CỨNG 12 VÒNG
            const res = Rubik4DCrypto.encrypt(text, key, 12);
            
            setElementData('cipherInput', res.hex);
            setElementData('cipherOutput', res.hex);

            // Đồng bộ trực tiếp sang ô nhập giải mã bên phải
            const decCipherInput = document.getElementById('decryptCipherInput');
            if (decCipherInput) decCipherInput.value = res.hex;

            if (res.firstBlock) renderTesseractGrid(res.firstBlock);
            showToast('Mã hóa thành công 12 vòng!');
        });
    }

    // 2. SỰ KIỆN GIẢI MÃ (CỐ ĐỊNH 12 VÒNG)
    if (decryptBtn) {
        decryptBtn.addEventListener('click', () => {
            const key = getElementData('decryptMasterKey') || getElementData('masterKey') || 'PTIT_ATTT_B24DCAT088';
            const hex = getElementData('decryptCipherInput') || getElementData('cipherInput');
            
            if (!key) return showToast('Vui lòng nhập Key!');
            if (!hex || hex.length % 32 !== 0) return showToast('Bản mã Hex không hợp lệ (phải chia hết cho 32 ký tự)!');

            try {
                // ÉP CỨNG 12 VÒNG
                const plain = Rubik4DCrypto.decrypt(hex, key, 12);
                
                setElementData('recoveredPlainInput', plain);
                setElementData('plainOutput', plain);
                setElementData('recoveredPlain', plain);

                showToast('Giải mã thành công 12 vòng!');
            } catch (err) {
                showToast(err.message);
            }
        });
    }

    if (clearPlainBtn && plainInput) {
        clearPlainBtn.addEventListener('click', () => plainInput.value = '');
    }

    if (copyCipherBtn && cipherInput) {
        copyCipherBtn.addEventListener('click', () => {
            const val = getElementData('cipherInput');
            if (!val) return;
            navigator.clipboard.writeText(val);
            showToast('Đã copy chuỗi Hex!');
        });
    }

    // 3. AVALANCHE BENCHMARK (12 VÒNG)
    if (runAvalancheBtn) {
        runAvalancheBtn.addEventListener('click', () => {
            const key = getElementData('masterKey') || 'PTIT_ATTT_B24DCAT088';
            const sample1 = new TextEncoder().encode("NguyenVanA_BaoCaoMonMatMaHoc2026");
            const sample2 = new Uint8Array(sample1);
            sample2[0] ^= 1; // Lật 1 bit

            const tbody = document.getElementById('avalancheTableBody');
            if (!tbody) return;
            tbody.innerHTML = '';

            for (let r = 1; r <= 12; r++) {
                const rkeys = Rubik4DCrypto.generateRoundKeys(key, r);
                const c1 = Rubik4DCrypto.encryptBlock(sample1.slice(0, 16), rkeys, r);
                const c2 = Rubik4DCrypto.encryptBlock(sample2.slice(0, 16), rkeys, r);

                let diffBits = 0;
                for (let i = 0; i < 16; i++) {
                    let xor = c1[i] ^ c2[i];
                    while (xor > 0) { diffBits += xor & 1; xor >>= 1; }
                }

                const ratio = ((diffBits / 128) * 100).toFixed(2);
                const statusColor = ratio > 40 && ratio < 60 ? 'text-emerald-400' : 'text-amber-400';

                tbody.innerHTML += `
                    <tr class="hover:bg-cyber-card/40">
                        <td class="py-3 px-4 font-bold text-white">Vòng ${r}</td>
                        <td class="py-3 px-4 text-cyan-400">${diffBits} / 128 bits</td>
                        <td class="py-3 px-4 font-bold ${statusColor}">${ratio}%</td>
                        <td class="py-3 px-4">
                            <div class="w-32 bg-cyber-bg rounded-full h-2 overflow-hidden border border-cyber-border">
                                <div class="bg-cyan-400 h-full" style="width: ${ratio}%"></div>
                            </div>
                        </td>
                    </tr>
                `;
            }
            showToast('Đã phân tích xong SAC 12 vòng!');
        });
    }

    // Helper functions
    function getElementData(id) {
        const el = document.getElementById(id);
        if (!el) return '';
        return (el.value !== undefined ? el.value : el.innerText || el.textContent || '').trim();
    }

    function setElementData(id, val) {
        const el = document.getElementById(id);
        if (!el) return;
        if (el.value !== undefined) el.value = val;
        el.innerText = val;
        el.textContent = val;
    }

    function renderTesseractGrid(bytes16) {
        const w0 = document.getElementById('grid-w0');
        const w1 = document.getElementById('grid-w1');
        if (!w0 || !w1 || !bytes16) return;
        w0.innerHTML = '';
        w1.innerHTML = '';

        for (let i = 0; i < 8; i++) {
            w0.innerHTML += `<div class="bg-cyber-bg p-3 rounded-lg border border-cyan-900/50 text-cyan-300 font-bold">0x${bytes16[i].toString(16).padStart(2,'0')}</div>`;
        }
        for (let i = 8; i < 16; i++) {
            w1.innerHTML += `<div class="bg-cyber-bg p-3 rounded-lg border border-purple-900/50 text-purple-300 font-bold">0x${bytes16[i].toString(16).padStart(2,'0')}</div>`;
        }
    }

    function showToast(msg) {
        const toast = document.getElementById('toast');
        const toastMsg = document.getElementById('toastMsg');
        if (!toast || !toastMsg) return;
        toastMsg.textContent = msg;
        toast.classList.remove('translate-y-20', 'opacity-0');
        setTimeout(() => toast.classList.add('translate-y-20', 'opacity-0'), 2500);
    }
});