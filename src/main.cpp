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
#include "simon128.h"
#include "chacha20.h"
#include "analysis.h"

#define NUM_TOTAL_ALGOS 5

static const char* ALGO_NAMES[NUM_TOTAL_ALGOS] = { 
    "Rubik-4D (SO(4)+ARX)", 
    "AES-128 (FIPS-197)", 
    "SPECK-128 (NSA ARX)",
    "SIMON-128 (NSA Feistel)",
    "ChaCha20 (RFC-8439)"
};

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

    const char* tags[] = { "_rubik4d", "_aes", "_speck", "_simon", "_chacha20" };
    const char* tag = (algo >= 0 && algo < NUM_TOTAL_ALGOS) ? tags[algo] : "_cipher";

    if (is_encrypt) {
        return base + tag + "_enc" + ext;
    } else {
        std::string enc_tag = std::string(tag) + "_enc";
        size_t tag_pos = base.rfind(enc_tag);
        if (tag_pos != std::string::npos) base.erase(tag_pos, enc_tag.length());
        return base + "_decrypted" + ext;
    }
}

static size_t DispatchEncrypt(int algo, const uint8_t* in, size_t in_len, uint8_t* out, const uint8_t* key, size_t key_len) {
    uint8_t default_iv[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 
                              0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    switch (algo) {
        case 0: return rubik4d_encrypt(in, in_len, out, key, key_len, default_iv);
        case 1: return aes128_encrypt(in, in_len, out, key, key_len, default_iv);
        case 2: return speck128_encrypt(in, in_len, out, key, key_len, default_iv);
        case 3: return simon128_encrypt(in, in_len, out, key, key_len, default_iv);
        case 4: return chacha20_encrypt(in, in_len, out, key, key_len, default_iv);
        default: return 0;
    }
}

static size_t DispatchDecrypt(int algo, const uint8_t* in, size_t in_len, uint8_t* out, const uint8_t* key, size_t key_len) {
    uint8_t default_iv[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 
                              0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    switch (algo) {
        case 0: return rubik4d_decrypt(in, in_len, out, key, key_len, default_iv);
        case 1: return aes128_decrypt(in, in_len, out, key, key_len, default_iv);
        case 2: return speck128_decrypt(in, in_len, out, key, key_len, default_iv);
        case 3: return simon128_decrypt(in, in_len, out, key, key_len, default_iv);
        case 4: return chacha20_decrypt(in, in_len, out, key, key_len, default_iv);
        default: return 0;
    }
}

// ==========================================
// STATE BENCHMARK SUITE
// ==========================================
struct BenchResult {
    int algo_id = -1;
    int iters = 0;
    double time_ms = 0.0;
    double throughput = 0.0;
    double cpb = 0.0;
    double entropy = 0.0;
    char status[64] = "Chua chay";

    void Reset() {
        algo_id = -1;
        iters = 0;
        time_ms = 0.0;
        throughput = 0.0;
        cpb = 0.0;
        entropy = 0.0;
        snprintf(status, sizeof(status), "Chua chay");
    }
};

static std::atomic<bool> is_benchmarking(false);
static std::atomic<bool> cancel_requested(false);
static std::atomic<int> current_active_algo(-1);
static std::atomic<int> progress_iters(0);
static int active_target_iters = 1000;

static BenchResult multi_results[NUM_TOTAL_ALGOS];
static bool benchmark_all_mode = true;
static int selected_single_algo = 0;

void MultiBenchmarkWorker(std::vector<uint8_t> input_bytes, std::string password, int iters, bool run_all, int single_target) {
    size_t in_len = input_bytes.size();
    size_t key_len = password.size();
    const uint8_t* key_ptr = (const uint8_t*)password.c_str();

    std::vector<uint8_t> out_buf(in_len + 64);
    int step = (in_len > 65536) ? 1 : std::max(1, iters / 100);

    int start_algo = run_all ? 0 : single_target;
    int end_algo   = run_all ? NUM_TOTAL_ALGOS : (single_target + 1);

    for (int a = start_algo; a < end_algo; a++) {
        if (cancel_requested.load()) break;

        current_active_algo = a;
        progress_iters = 0;

        uint64_t c0 = __rdtsc();
        auto t0 = std::chrono::high_resolution_clock::now();

        for (int i = 1; i <= iters; i++) {
            if (cancel_requested.load()) break;
            DispatchEncrypt(a, input_bytes.data(), in_len, out_buf.data(), key_ptr, key_len);
            if (i % step == 0 || i == iters) progress_iters = i;
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        uint64_t c1 = __rdtsc();

        if (cancel_requested.load()) break;

        multi_results[a].time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        double sec = multi_results[a].time_ms / 1000.0;
        double mb = ((double)in_len * iters) / (1024.0 * 1024.0);
        multi_results[a].throughput = (sec > 0.0) ? (mb / sec) : 0.0;
        multi_results[a].cpb = (double)(c1 - c0) / ((double)in_len * iters);

        size_t out_len = DispatchEncrypt(a, input_bytes.data(), in_len, out_buf.data(), key_ptr, key_len);
        multi_results[a].entropy = rubik4d_calculate_entropy(out_buf.data(), out_len);

        std::vector<uint8_t> dec_buf(out_len + 32);
        size_t dec_len = DispatchDecrypt(a, out_buf.data(), out_len, dec_buf.data(), key_ptr, key_len);

        if (dec_len == in_len && memcmp(dec_buf.data(), input_bytes.data(), in_len) == 0) {
            snprintf(multi_results[a].status, sizeof(multi_results[a].status), "Toan ven 100%%");
        } else {
            snprintf(multi_results[a].status, sizeof(multi_results[a].status), "Loi toan ven!");
        }

        multi_results[a].algo_id = a;
        multi_results[a].iters = iters;
    }

    current_active_algo = -1;
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

    GLFWwindow* window = glfwCreateWindow(1280, 880, "Rubik-4D Native Cryptographic Suite & Benchmark", NULL, NULL);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesVietnamese());

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.ItemSpacing = ImVec2(10, 8);
    style.CellPadding = ImVec2(8, 6);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    rubik4d_init_tables();

    static int bm_data_type = 1;
    static char bm_text[4096] = "Lightweight Block Cipher Benchmark Data.";
    static char bm_filepath[512] = "";
    static char bm_password[128] = "Rubik4DMasterKey128B";
    static int input_iters = 1000;

    static int formal_algo = 0;
    static char formal_in_path[512] = "";
    static char formal_out_path[512] = "";
    static char formal_password[128] = "Rubik4DMasterKey128B";
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

        ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "RUBIK-4D: UNIVERSAL CRYPTOGRAPHIC BENCHMARK & COMPARISON SUITE");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("MainTabs")) {
            
            // ==================== TAB 1: BENCHMARK TỔNG HỢP ====================
            if (ImGui::BeginTabItem("[1] Benchmark So Sanh")) {
                ImGui::Spacing();
                ImGui::BeginDisabled(is_benchmarking.load());

                // Tùy chọn chế độ đo
                ImGui::Text("Che do kiem thu:"); ImGui::SameLine();
                ImGui::RadioButton("So sanh tat ca 5 thuat toan", (int*)&benchmark_all_mode, 1); ImGui::SameLine();
                ImGui::RadioButton("Chi chay 1 thuat toan", (int*)&benchmark_all_mode, 0);

                if (!benchmark_all_mode) {
                    ImGui::SetNextItemWidth(300);
                    ImGui::Combo("##single_algo_pick", &selected_single_algo, ALGO_NAMES, NUM_TOTAL_ALGOS);
                }

                ImGui::Spacing();
                ImGui::RadioButton("Du lieu Text", &bm_data_type, 0); ImGui::SameLine();
                ImGui::RadioButton("Du lieu File (Bin/PDF/Img/Video)", &bm_data_type, 1);

                if (bm_data_type == 0) {
                    ImGui::InputTextMultiline("##bm_text", bm_text, sizeof(bm_text), ImVec2(-1, 50));
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
                    ImGui::Text("So vong lap (Iters):");
                    ImGui::SetNextItemWidth(-1);
                    ImGui::InputInt("##bm_iters", &input_iters);
                    if (input_iters < 1) input_iters = 1;
                    ImGui::EndTable();
                }
                ImGui::EndDisabled();

                ImGui::Spacing();

                // Nút kích hoạt
                if (is_benchmarking.load()) {
                    if (ImGui::BeginTable("tbl_ctrl_running", 2, ImGuiTableFlags_SizingStretchProp)) {
                        ImGui::TableSetupColumn("c1", ImGuiTableColumnFlags_WidthStretch, 0.82f);
                        ImGui::TableSetupColumn("c2", ImGuiTableColumnFlags_WidthStretch, 0.18f);
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        int cur = current_active_algo.load();
                        char prog_lbl[128];
                        snprintf(prog_lbl, sizeof(prog_lbl), "DANG CHAY: %s (%d / %d)...", 
                                 (cur >= 0 ? ALGO_NAMES[cur] : "Khoi tao"), progress_iters.load(), active_target_iters);
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.35f, 0.45f, 1.0f));
                        ImGui::Button(prog_lbl, ImVec2(-1, 38));
                        ImGui::PopStyleColor();

                        ImGui::TableSetColumnIndex(1);
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                        if (ImGui::Button("DUNG BENCHMARK", ImVec2(-1, 38))) cancel_requested = true;
                        ImGui::PopStyleColor();
                        ImGui::EndTable();
                    }
                } else {
                    char btn_label[128];
                    if (benchmark_all_mode) {
                        snprintf(btn_label, sizeof(btn_label), "CHAY BENCHMARK DONG LOAT TRON BO 5 THUAT TOAN");
                    } else {
                        snprintf(btn_label, sizeof(btn_label), "CHAY BENCHMARK CHO: [%s]", ALGO_NAMES[selected_single_algo]);
                    }

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.52f, 0.38f, 1.0f));
                    if (ImGui::Button(btn_label, ImVec2(-1, 40))) {
                        std::vector<uint8_t> in_bytes;
                        if (bm_data_type == 0) {
                            in_bytes.assign((uint8_t*)bm_text, (uint8_t*)bm_text + strlen(bm_text));
                        } else {
                            ReadBinaryFile(bm_filepath, in_bytes);
                        }

                        if (!in_bytes.empty() && strlen(bm_password) > 0) {
                            is_benchmarking = true;
                            cancel_requested = false;
                            active_target_iters = input_iters;

                            if (benchmark_all_mode) {
                                for (int i = 0; i < NUM_TOTAL_ALGOS; i++) multi_results[i].Reset();
                            } else {
                                multi_results[selected_single_algo].Reset();
                            }

                            std::thread th(MultiBenchmarkWorker, in_bytes, std::string(bm_password), 
                                           active_target_iters, benchmark_all_mode, selected_single_algo);
                            th.detach();
                        }
                    }
                    ImGui::PopStyleColor();
                }

                // Progress Bar
                float progress = (active_target_iters > 0) ? (float)progress_iters.load() / (float)active_target_iters : 0.0f;
                ImGui::ProgressBar(progress, ImVec2(-1, 15));

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "BANG SO SANH TONG HOP DANG COT (METRICS COMPARISON TABLE):");
                ImGui::Spacing();

                // BẢNG SO SÁNH DẠNG CỘT CHO TRỌN BỘ CÁC THUẬT TOÁN
                if (ImGui::BeginTable("tbl_multi_compare", 6, 
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame)) {
                    
                    ImGui::TableSetupColumn("Thuat toan");
                    ImGui::TableSetupColumn("Thoi gian (ms)");
                    ImGui::TableSetupColumn("Throughput");
                    ImGui::TableSetupColumn("Cycles/Byte");
                    ImGui::TableSetupColumn("Entropy");
                    ImGui::TableSetupColumn("Tinh toan ven");
                    ImGui::TableHeadersRow();

                    for (int a = 0; a < NUM_TOTAL_ALGOS; a++) {
                        ImGui::TableNextRow();

                        // Cột 1: Tên thuật toán
                        ImGui::TableSetColumnIndex(0);
                        if (a == 0) {
                            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "* %s", ALGO_NAMES[a]);
                        } else {
                            ImGui::Text("%s", ALGO_NAMES[a]);
                        }
                        if (current_active_algo.load() == a) {
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[Dang do...]");
                        }

                        // Cột 2: Thời gian
                        ImGui::TableSetColumnIndex(1);
                        if (multi_results[a].algo_id != -1) {
                            ImGui::Text("%.2f ms", multi_results[a].time_ms);
                        } else {
                            ImGui::TextDisabled("-");
                        }

                        // Cột 3: Throughput
                        ImGui::TableSetColumnIndex(2);
                        if (multi_results[a].algo_id != -1) {
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 1.0f), "%.2f MB/s", multi_results[a].throughput);
                        } else {
                            ImGui::TextDisabled("-");
                        }

                        // Cột 4: Cycles per Byte
                        ImGui::TableSetColumnIndex(3);
                        if (multi_results[a].algo_id != -1) {
                            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%.2f cpb", multi_results[a].cpb);
                        } else {
                            ImGui::TextDisabled("-");
                        }

                        // Cột 5: Entropy
                        ImGui::TableSetColumnIndex(4);
                        if (multi_results[a].algo_id != -1) {
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%.4f", multi_results[a].entropy);
                        } else {
                            ImGui::TextDisabled("-");
                        }

                        // Cột 6: Tình trạng toàn vẹn
                        ImGui::TableSetColumnIndex(5);
                        if (multi_results[a].algo_id != -1) {
                            if (strstr(multi_results[a].status, "100%")) {
                                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", multi_results[a].status);
                            } else {
                                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", multi_results[a].status);
                            }
                        } else {
                            ImGui::TextDisabled("-");
                        }
                    }
                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            // ==================== TAB 2: FORMAL USE ====================
            if (ImGui::BeginTabItem("[2] Formal Use")) {
                ImGui::Spacing();
                ImGui::Text("Thuat toan thuc thi:");
                ImGui::Combo("##formal_algo_combo", &formal_algo, ALGO_NAMES, NUM_TOTAL_ALGOS);
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
                            std::vector<uint8_t> out_buf(in_buf.size() + 64);
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
                            snprintf(formal_log, sizeof(formal_log), "Loi: Khong tim thay file can giai ma!");
                            formal_log_color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                        } else {
                            rubik4d_init_tables();
                            size_t key_len = strlen(formal_password);
                            std::vector<uint8_t> out_buf(in_buf.size() + 64);
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
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "PHAN TICH KHUECH TAN SAC, NPCR & UACI (1000 SAMPLES)");
                ImGui::Spacing();

                if (is_sec_running.load()) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
                    ImGui::Button("DANG CHAY KIEM DINH THONG KE (1000 SAMPLES)...", ImVec2(-1, 40));
                    ImGui::PopStyleColor();
                } else {
                    if (ImGui::Button("CHAY KIEM DINH THONG KE TRON BO", ImVec2(-1, 40))) {
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
                    ImGui::TextDisabled("(Ky vong: > 99.6%%)");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("NPCR: %.4f %%", sec_aes.npcr);
                    ImGui::TextDisabled("(Ky vong: > 99.6%%)");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("NPCR: %.4f %%", sec_speck.npcr);
                    ImGui::TextDisabled("(Ky vong: > 99.6%%)");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("UACI: %.4f %%", sec_rubik.uaci);
                    ImGui::TextDisabled("(Ky vong: ~ 33.4%%)");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("UACI: %.4f %%", sec_aes.uaci);
                    ImGui::TextDisabled("(Ky vong: ~ 33.4%%)");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("UACI: %.4f %%", sec_speck.uaci);
                    ImGui::TextDisabled("(Ky vong: ~ 33.4%%)");

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