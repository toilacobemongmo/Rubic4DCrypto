const text = "Hello Tesseract 4D!";
const pass = "PTIT_ATTT_B24DCAT088";

// Test trực tiếp không qua DOM HTML
const encrypted = Rubik4DCrypto.encrypt(text, pass);
console.log("Ciphertext:", encrypted.hex);

const decrypted = Rubik4DCrypto.decrypt(encrypted.hex, pass);
console.log("Decrypted:", decrypted); 
console.log("Thành công?", text === decrypted);