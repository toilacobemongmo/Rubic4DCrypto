#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <vector>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <algorithm>

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include "rubik4d.h"
#include "aes128.h"
#include "speck128.h"
#include "analysis.h"

static const char* ALGO_NAMES[] = { "Rubik-4D (SO(4)+ARX)", "AES-128 (FIPS-197)", "SPECK-128 (NSA ARX)" };

static bool ReadBinaryFile(const char* path, std::vector<uint8_t>& out_data) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    out_data.resize(size);
    return (bool)file.read((char*)out_data.data(), size);
}

static bool WriteBinaryFile(const char* path, const std::vector<uint8_t>& in_data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file.write((const char*)in_data.data(), in_data.size());
    return true;
}

static std::string GenerateSmartPath(const std::string& path, int algo, bool is_encrypt) {
    size_t dot_pos = path.find_last_of('.');
    std::string base = (dot_pos == std::string::npos) ? path : path.substr(0, dot_pos);
    std::string ext = (dot_pos == std::string::npos) ? "" : path.substr(dot_pos);

    const char* tag = (algo == 0) ? "_rubik4d" : (algo == 1 ? "_aes" : "_speck");

    if (is_encrypt) {
        return base + tag + "_enc" + ext;
    } else {
        std::string enc_tag = std::string(tag) + "_enc";
        size_t tag_pos = base.rfind(enc_tag);
        if (tag_pos != std::string::npos) {
            base.erase(tag_pos, enc_tag.length());
        }
        return base + "_decrypted" + ext;
    }
}

static size_t DispatchEncrypt(int algo, const uint8_t* in, size_t in_len, uint8_t* out, const uint8_t* key, size_t key_len) {
    if (algo == 0) {
        uint8_t default_iv[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 
                                  0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
        return rubik4d_encrypt(in, in_len, out, key, key_len, default_iv);
    }
    if (algo == 1) return aes128_encrypt(in, in_len, out, key, key_len);
    return speck128_encrypt(in, in_len, out, key, key_len);
}

static size_t DispatchDecrypt(int algo, const uint8_t* in, size_t in_len, uint8_t* out, const uint8_t* key, size_t key_len) {
    if (algo == 0) {
        uint8_t default_iv[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 
                                  0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
        return rubik4d_decrypt(in, in_len, out, key, key_len, default_iv);
    }
    if (algo == 1) return aes128_decrypt(in, in_len, out, key, key_len);
    return speck128_decrypt(in, in_len, out, key, key_len); // Đã sửa thành decrypt chuẩn xác
}

// ==========================================
// STATE BENCHMARK THREAD
// ==========================================
struct BenchResult {
    int algo_id = -1;
    int iters = 0;
    double time_ms = 0.0;
    double throughput = 0.0;
    double cpb = 0.0;
    double entropy = 0.0;
    char hex[128] = "Chua chay";
    char status[64] = "Chua chay";

    void Reset() {
        algo_id = -1;
        iters = 0;
        time_ms = 0.0;
        throughput = 0.0;
        cpb = 0.0;
        entropy = 0.0;
        snprintf(hex, sizeof(hex), "Chua chay");
        snprintf(status, sizeof(status), "Chua chay");
    }
};

static std::atomic<bool> is_benchmarking(false);
static std::atomic<bool> cancel_requested(false);
static std::atomic<int> current_active_slot(0);
static std::atomic<int> iter_slot_a(0);
static std::atomic<int> iter_slot_b(0);
static int active_target_iters = 5000;

static int selected_algo_a = 0;
static int selected_algo_b = 2;
static int last_algo_a = 0;
static int last_algo_b = 2;
static BenchResult res_a, res_b;

void BenchmarkWorker(std::vector<uint8_t> input_bytes, std::string password, int iters, int algo_a, int algo_b) {
    size_t in_len = input_bytes.size();
    size_t key_len = password.size();
    const uint8_t* key_ptr = (const uint8_t*)password.c_str();

    std::vector<uint8_t> out_buf(in_len + 32);
    int step = (in_len > 65536) ? 1 : std::max(1, iters / 200);

    // Slot A
    current_active_slot = 1;
    uint64_t c0_a = __rdtsc();
    auto t0_a = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= iters; i++) {
        if (cancel_requested.load()) { is_benchmarking = false; current_active_slot = 0; return; }
        DispatchEncrypt(algo_a, input_bytes.data(), in_len, out_buf.data(), key_ptr, key_len);
        if (i % step == 0 || i == iters) iter_slot_a = i;
    }
    auto t1_a = std::chrono::high_resolution_clock::now();
    uint64_t c1_a = __rdtsc();

    res_a.time_ms = std::chrono::duration<double, std::milli>(t1_a - t0_a).count();
    double sec_a = res_a.time_ms / 1000.0;
    double mb_a = ((double)in_len * iters) / (1024.0 * 1024.0);
    res_a.throughput = (sec_a > 0.0) ? (mb_a / sec_a) : 0.0;
    res_a.cpb = (double)(c1_a - c0_a) / ((double)in_len * iters);

    size_t out_len_a = DispatchEncrypt(algo_a, input_bytes.data(), in_len, out_buf.data(), key_ptr, key_len);
    res_a.entropy = rubik4d_calculate_entropy(out_buf.data(), out_len_a);

    res_a.hex[0] = '\0';
    for (size_t i = 0; i < out_len_a && i < 16; i++) {
        char b[4]; snprintf(b, sizeof(b), "%02x ", out_buf[i]); strcat(res_a.hex, b);
    }
    strcat(res_a.hex, "...");

    std::vector<uint8_t> dec_a(out_len_a + 16);
    size_t dec_len_a = DispatchDecrypt(algo_a, out_buf.data(), out_len_a, dec_a.data(), key_ptr, key_len);
    snprintf(res_a.status, sizeof(res_a.status), (dec_len_a == in_len && memcmp(dec_a.data(), input_bytes.data(), in_len) == 0) ? "Toan ven 100%%" : "Loi toan ven!");
    res_a.algo_id = algo_a;
    res_a.iters = iters;

    if (cancel_requested.load()) { is_benchmarking = false; current_active_slot = 0; return; }

    // Slot B
    current_active_slot = 2;
    uint64_t c0_b = __rdtsc();
    auto t0_b = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= iters; i++) {
        if (cancel_requested.load()) { is_benchmarking = false; current_active_slot = 0; return; }
        DispatchEncrypt(algo_b, input_bytes.data(), in_len, out_buf.data(), key_ptr, key_len);
        if (i % step == 0 || i == iters) iter_slot_b = i;
    }
    auto t1_b = std::chrono::high_resolution_clock::now();
    uint64_t c1_b = __rdtsc();

    res_b.time_ms = std::chrono::duration<double, std::milli>(t1_b - t0_b).count();
    double sec_b = res_b.time_ms / 1000.0;
    double mb_b = ((double)in_len * iters) / (1024.0 * 1024.0);
    res_b.throughput = (sec_b > 0.0) ? (mb_b / sec_b) : 0.0;
    res_b.cpb = (double)(c1_b - c0_b) / ((double)in_len * iters);

    size_t out_len_b = DispatchEncrypt(algo_b, input_bytes.data(), in_len, out_buf.data(), key_ptr, key_len);
    res_b.entropy = rubik4d_calculate_entropy(out_buf.data(), out_len_b);

    res_b.hex[0] = '\0';
    for (size_t i = 0; i < out_len_b && i < 16; i++) {
        char b[4]; snprintf(b, sizeof(b), "%02x ", out_buf[i]); strcat(res_b.hex, b);
    }
    strcat(res_b.hex, "...");

    std::vector<uint8_t> dec_b(out_len_b + 16);
    size_t dec_len_b = DispatchDecrypt(algo_b, out_buf.data(), out_len_b, dec_b.data(), key_ptr, key_len);
    snprintf(res_b.status, sizeof(res_b.status), (dec_len_b == in_len && memcmp(dec_b.data(), input_bytes.data(), in_len) == 0) ? "Toan ven 100%%" : "Loi toan ven!");
    res_b.algo_id = algo_b;
    res_b.iters = iters;

    current_active_slot = 0;
    is_benchmarking = false;
}

// State Security Analysis Tab
static std::atomic<bool> is_sec_running(false);
static SecurityMetrics sec_rubik, sec_aes, sec_speck;
static char sec_status[128] = "Chua chay phan tich.";

int main() {
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1180, 850, "Universal Native Cryptographic Suite", NULL, NULL);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    ImFont* sys_font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesVietnamese());
    if (!sys_font) {
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesVietnamese());
    }

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.ItemSpacing = ImVec2(10, 8);
    style.CellPadding = ImVec2(8, 6);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    rubik4d_init_tables();

    static int bm_data_type = 1;
    static char bm_text[4096] = "Test Vector Plaintext.";
    static char bm_filepath[512] = "C:\\Users\\ghaob\\Downloads\\ma_niessen_rubik.pdf";
    static char bm_password[128] = "MasterPassword128BitKey";
    static int input_iters = 5000;

    static int formal_algo = 0;
    static char formal_in_path[512] = "";
    static char formal_out_path[512] = "";
    static char formal_password[128] = "MasterPassword128BitKey";
    static char formal_log[256] = "San sang.";
    static ImVec4 formal_log_color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));

        ImGui::Begin("Workspace", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.95f, 1.0f), "UNIVERSAL CRYPTOGRAPHIC BENCHMARK & SECURITY SUITE");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("MainTabs")) {
            
            // ==================== TAB 1: BENCHMARK ====================
            if (ImGui::BeginTabItem("[1] Benchmark So Sanh")) {
                ImGui::Spacing();
                ImGui::BeginDisabled(is_benchmarking.load());

                if (ImGui::BeginTable("tbl_selectors", 2, ImGuiTableFlags_SizingStretchSame)) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Thuat toan Ve Trai (Slot A):");
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::Combo("##algo_a", &selected_algo_a, ALGO_NAMES, 3)) {
                        if (selected_algo_a != last_algo_a) {
                            res_a.Reset();
                            iter_slot_a = 0;
                            last_algo_a = selected_algo_a;
                        }
                    }

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Thuat toan Ve Phai (Slot B):");
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::Combo("##algo_b", &selected_algo_b, ALGO_NAMES, 3)) {
                        if (selected_algo_b != last_algo_b) {
                            res_b.Reset();
                            iter_slot_b = 0;
                            last_algo_b = selected_algo_b;
                        }
                    }
                    ImGui::EndTable();
                }

                ImGui::Spacing();
                ImGui::RadioButton("Du lieu Text", &bm_data_type, 0); ImGui::SameLine();
                ImGui::RadioButton("Du lieu File (Bin/PDF/Img)", &bm_data_type, 1);

                if (bm_data_type == 0) {
                    ImGui::InputTextMultiline("##bm_text", bm_text, sizeof(bm_text), ImVec2(-1, 60));
                } else {
                    ImGui::InputText("##bm_filepath", bm_filepath, sizeof(bm_filepath));
                }

                if (ImGui::BeginTable("tbl_bm_cfg", 2, ImGuiTableFlags_SizingStretchSame)) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Mat khau:");
                    ImGui::SetNextItemWidth(-1);
                    ImGui::InputText("##bm_pass", bm_password, sizeof(bm_password));

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("So lan lap:");
                    ImGui::SetNextItemWidth(-1);
                    ImGui::InputInt("##bm_iters", &input_iters);
                    if (input_iters < 1) input_iters = 1;
                    ImGui::EndTable();
                }
                ImGui::EndDisabled();

                ImGui::Spacing();

                if (is_benchmarking.load()) {
                    if (ImGui::BeginTable("tbl_ctrl_running", 2, ImGuiTableFlags_SizingStretchProp)) {
                        ImGui::TableSetupColumn("c1", ImGuiTableColumnFlags_WidthStretch, 0.8f);
                        ImGui::TableSetupColumn("c2", ImGuiTableColumnFlags_WidthStretch, 0.2f);
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.25f, 0.35f, 1.0f));
                        ImGui::Button("DANG BENCHMARK TREN BACKGROUND THREAD...", ImVec2(-1, 40));
                        ImGui::PopStyleColor();

                        ImGui::TableSetColumnIndex(1);
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                        if (ImGui::Button("HUY NGAY", ImVec2(-1, 40))) cancel_requested = true;
                        ImGui::PopStyleColor();
                        ImGui::EndTable();
                    }
                } else {
                    char btn_label[128];
                    snprintf(btn_label, sizeof(btn_label), "CHAY SO SANH: [%s]  VS  [%s]", ALGO_NAMES[selected_algo_a], ALGO_NAMES[selected_algo_b]);
                    if (ImGui::Button(btn_label, ImVec2(-1, 40))) {
                        std::vector<uint8_t> in_bytes;
                        if (bm_data_type == 0) in_bytes.assign((uint8_t*)bm_text, (uint8_t*)bm_text + strlen(bm_text));
                        else ReadBinaryFile(bm_filepath, in_bytes);

                        if (!in_bytes.empty() && strlen(bm_password) > 0) {
                            is_benchmarking = true;
                            cancel_requested = false;
                            active_target_iters = input_iters;

                            iter_slot_a = 0;
                            iter_slot_b = 0;
                            res_a.Reset();
                            res_b.Reset();

                            std::thread th(BenchmarkWorker, in_bytes, std::string(bm_password), active_target_iters, selected_algo_a, selected_algo_b);
                            th.detach();
                        }
                    }
                }

                // Progress Bar
                int target = active_target_iters;
                float ratio_a = (target > 0) ? (float)iter_slot_a.load() / (float)target : 0.0f;
                char txt_a[64]; snprintf(txt_a, sizeof(txt_a), "%s: %d / %d", ALGO_NAMES[selected_algo_a], iter_slot_a.load(), target);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.25f, 0.55f, 0.9f, 1.0f));
                ImGui::ProgressBar(ratio_a, ImVec2(-1, 18), txt_a);
                ImGui::PopStyleColor();

                float ratio_b = (target > 0) ? (float)iter_slot_b.load() / (float)target : 0.0f;
                char txt_b[64]; snprintf(txt_b, sizeof(txt_b), "%s: %d / %d", ALGO_NAMES[selected_algo_b], iter_slot_b.load(), target);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.25f, 0.85f, 0.35f, 1.0f));
                ImGui::ProgressBar(ratio_b, ImVec2(-1, 18), txt_b);
                ImGui::PopStyleColor();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::BeginTable("tbl_compare_results", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchSame)) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(ImVec4(0.35f, 0.65f, 1.0f, 1.0f), "%s", ALGO_NAMES[selected_algo_a]);
                    if (current_active_slot == 1) { ImGui::SameLine(); ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "[Dang chay...]"); }
                    ImGui::Separator();

                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImVec4(0.25f, 0.9f, 0.4f, 1.0f), "%s", ALGO_NAMES[selected_algo_b]);
                    if (current_active_slot == 2) { ImGui::SameLine(); ImGui::TextColored(ImVec4(0.2f, 1, 0.2f, 1), "[Dang chay...]"); }
                    ImGui::Separator();

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    if (res_a.algo_id == selected_algo_a) ImGui::Text("Thoi gian: %.2f ms (%d lan)", res_a.time_ms, res_a.iters);
                    else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Chua co ket qua");

                    ImGui::TableSetColumnIndex(1);
                    if (res_b.algo_id == selected_algo_b) ImGui::Text("Thoi gian: %.2f ms (%d lan)", res_b.time_ms, res_b.iters);
                    else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Chua co ket qua");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Throughput: "); ImGui::SameLine();
                    if (res_a.algo_id == selected_algo_a) ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 1.0f), "%.2f MB/s", res_a.throughput);
                    else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Throughput: "); ImGui::SameLine();
                    if (res_b.algo_id == selected_algo_b) ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 1.0f), "%.2f MB/s", res_b.throughput);
                    else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Cycles/Byte: "); ImGui::SameLine();
                    if (res_a.algo_id == selected_algo_a) ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%.2f cpb", res_a.cpb);
                    else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Cycles/Byte: "); ImGui::SameLine();
                    if (res_b.algo_id == selected_algo_b) ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%.2f cpb", res_b.cpb);
                    else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Entropy: "); ImGui::SameLine();
                    if (res_a.algo_id == selected_algo_a) ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%.4f / 8.0000", res_a.entropy);
                    else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Entropy: "); ImGui::SameLine();
                    if (res_b.algo_id == selected_algo_b) ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%.4f / 8.0000", res_b.entropy);
                    else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Preview: %s", (res_a.algo_id == selected_algo_a) ? res_a.hex : "-");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Preview: %s", (res_b.algo_id == selected_algo_b) ? res_b.hex : "-");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Toan ven: "); ImGui::SameLine();
                    if (res_a.algo_id == selected_algo_a) ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s", res_a.status);
                    else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Toan ven: "); ImGui::SameLine();
                    if (res_b.algo_id == selected_algo_b) ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s", res_b.status);
                    else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            // ==================== TAB 2: FORMAL USE ====================
            if (ImGui::BeginTabItem("[2] Formal Use")) {
                ImGui::Spacing();
                ImGui::Text("Thuat toan thuc thi:");
                ImGui::Combo("##formal_algo_combo", &formal_algo, ALGO_NAMES, 3);
                ImGui::Spacing();

                ImGui::Text("File goc can xu ly:");
                ImGui::InputText("##formal_fin", formal_in_path, sizeof(formal_in_path));

                ImGui::Text("File luu ra (de trong neu luu tu dong):");
                ImGui::InputText("##formal_fout", formal_out_path, sizeof(formal_out_path));

                ImGui::Text("Mat khau bao mat:");
                ImGui::InputText("##formal_fpass", formal_password, sizeof(formal_password));
                ImGui::Spacing();

                if (ImGui::BeginTable("tbl_formal_actions", 2, ImGuiTableFlags_SizingStretchSame)) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    if (ImGui::Button("Ma hoa File", ImVec2(-1, 40))) {
                        std::vector<uint8_t> in_buf;
                        if (!ReadBinaryFile(formal_in_path, in_buf)) {
                            snprintf(formal_log, sizeof(formal_log), "Loi: Khong tim thay file nguon!");
                            formal_log_color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                        } else {
                            rubik4d_init_tables();
                            size_t key_len = strlen(formal_password);
                            std::vector<uint8_t> out_buf(in_buf.size() + 32);
                            size_t out_len = DispatchEncrypt(formal_algo, in_buf.data(), in_buf.size(), out_buf.data(), (const uint8_t*)formal_password, key_len);
                            out_buf.resize(out_len);

                            std::string target = formal_out_path[0] ? formal_out_path : GenerateSmartPath(formal_in_path, formal_algo, true);
                            if (WriteBinaryFile(target.c_str(), out_buf)) {
                                snprintf(formal_log, sizeof(formal_log), "Da ma hoa thanh cong -> %s", target.c_str());
                                formal_log_color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
                            } else {
                                snprintf(formal_log, sizeof(formal_log), "Loi: Khong the ghi file ma hoa!");
                                formal_log_color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                            }
                        }
                    }

                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::Button("Giai ma File", ImVec2(-1, 40))) {
                        std::vector<uint8_t> in_buf;
                        if (!ReadBinaryFile(formal_in_path, in_buf)) {
                            snprintf(formal_log, sizeof(formal_log), "Loi: Khong tim thay file can giai ma! Kiem tra lai ten file.");
                            formal_log_color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                        } else if (in_buf.empty() || (in_buf.size() % 16) != 0) {
                            snprintf(formal_log, sizeof(formal_log), "Loi: File khong hop le (kich thuoc khong chia het cho 16)!");
                            formal_log_color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                        } else {
                            rubik4d_init_tables();
                            size_t key_len = strlen(formal_password);
                            std::vector<uint8_t> out_buf(in_buf.size() + 32);
                            size_t out_len = DispatchDecrypt(formal_algo, in_buf.data(), in_buf.size(), out_buf.data(), (const uint8_t*)formal_password, key_len);

                            if (out_len == 0) {
                                snprintf(formal_log, sizeof(formal_log), "Loi: Sai mat khau hoac sai thuat toan goc!");
                                formal_log_color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                            } else {
                                out_buf.resize(out_len);
                                std::string target = formal_out_path[0] ? formal_out_path : GenerateSmartPath(formal_in_path, formal_algo, false);
                                if (WriteBinaryFile(target.c_str(), out_buf)) {
                                    snprintf(formal_log, sizeof(formal_log), "Da giai ma thanh cong 100%% -> %s", target.c_str());
                                    formal_log_color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
                                } else {
                                    snprintf(formal_log, sizeof(formal_log), "Loi: Khong the ghi file giai ma!");
                                    formal_log_color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                                }
                            }
                        }
                    }
                    ImGui::EndTable();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Text("Nhat ky thuc thi:");
                ImGui::TextColored(formal_log_color, "%s", formal_log);

                ImGui::EndTabItem();
            }

            // ==================== TAB 3: SECURITY ANALYSIS ====================
            if (ImGui::BeginTabItem("[3] Security Analysis")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "PHAN TICH KHUECH TAN SAC, NPCR & UACI TRON BO 3 THUAT TOAN");
                ImGui::Spacing();

                if (is_sec_running.load()) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
                    ImGui::Button("DANG CHAY KIEM DINH THONG KE (1000 SAMPLES)...", ImVec2(-1, 40));
                    ImGui::PopStyleColor();
                } else {
                    if (ImGui::Button("CHAY KIEM DINH THONG KE TRON BO (1000 SAMPLES)", ImVec2(-1, 40))) {
                        is_sec_running = true;
                        std::thread sec_th([]() {
                            sec_rubik = AnalyzeCipherSecurity(ALGO_RUBIK4D, 1000);
                            sec_aes   = AnalyzeCipherSecurity(ALGO_AES128, 1000);
                            sec_speck = AnalyzeCipherSecurity(ALGO_SPECK128, 1000);
                            snprintf(sec_status, sizeof(sec_status), "Hoan thanh kiem dinh 1000 samples!");
                            is_sec_running = false;
                        });
                        sec_th.detach();
                    }
                }

                ImGui::Spacing();
                ImGui::Text("Trang thai: %s", sec_status);
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::BeginTable("tbl_sec_3cols", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchSame)) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(ImVec4(0.25f, 0.9f, 0.4f, 1.0f), "%s", ALGO_NAMES[0]);
                    ImGui::Separator();

                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImVec4(0.35f, 0.65f, 1.0f, 1.0f), "%s", ALGO_NAMES[1]);
                    ImGui::Separator();

                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%s", ALGO_NAMES[2]);
                    ImGui::Separator();

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("SAC: %.3f %%", sec_rubik.sac);
                    ImGui::TextDisabled("(Ly tuong: 50.0%%)");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("SAC: %.3f %%", sec_aes.sac);
                    ImGui::TextDisabled("(Ly tuong: 50.0%%)");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("SAC: %.3f %%", sec_speck.sac);
                    ImGui::TextDisabled("(Ly tuong: 50.0%%)");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("NPCR: %.4f %%", sec_rubik.npcr);
                    ImGui::TextDisabled("(Kỳ vong: > 99.6%%)");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("NPCR: %.4f %%", sec_aes.npcr);
                    ImGui::TextDisabled("(Kỳ vong: > 99.6%%)");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("NPCR: %.4f %%", sec_speck.npcr);
                    ImGui::TextDisabled("(Kỳ vong: > 99.6%%)");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("UACI: %.4f %%", sec_rubik.uaci);
                    ImGui::TextDisabled("(Kỳ vong: ~ 33.4%%)");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("UACI: %.4f %%", sec_aes.uaci);
                    ImGui::TextDisabled("(Kỳ vong: ~ 33.4%%)");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("UACI: %.4f %%", sec_speck.uaci);
                    ImGui::TextDisabled("(Kỳ vong: ~ 33.4%%)");

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.08f, 0.10f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}