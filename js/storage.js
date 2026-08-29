/**
 * Module quản trị LocalStorage tự động lưu mật khẩu người dùng
 */
const AppStorage = (() => {
    const KEY_NAME = 'rubik4d_master_key';

    function saveKey(key) {
        localStorage.setItem(KEY_NAME, key);
    }

    function loadKey() {
        return localStorage.getItem(KEY_NAME) || 'MatKhauBaoCao2026';
    }

    return { saveKey, loadKey };
})();