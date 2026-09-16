import pulp

# 1. KHỞI TẠO BẢNG HOÁN VỊ SO(4) Y HỆT CODE C
def rot2d(u, v, cw):
    if cw:
        if u == 0 and v == 0: return 0, 1
        elif u == 0 and v == 1: return 1, 1
        elif u == 1 and v == 1: return 1, 0
        else: return 0, 0
    else:
        if u == 0 and v == 0: return 1, 0
        elif u == 1 and v == 0: return 1, 1
        elif u == 1 and v == 1: return 0, 1
        else: return 0, 0

def init_perm_tables():
    perm_tables = []
    for p in range(6):
        for d in range(2):
            cw = (d == 0)
            perm = [0] * 16
            for i in range(16):
                x = (i >> 3) & 1
                y = (i >> 2) & 1
                z = (i >> 1) & 1
                w = i & 1
                if p == 0: x, y = rot2d(x, y, cw)
                elif p == 1: x, z = rot2d(x, z, cw)
                elif p == 2: x, w = rot2d(x, w, cw)
                elif p == 3: y, z = rot2d(y, z, cw)
                elif p == 4: y, w = rot2d(y, w, cw)
                elif p == 5: z, w = rot2d(z, w, cw)
                dest = (x << 3) | (y << 2) | (z << 1) | w
                perm[dest] = i
            perm_tables.append(perm)
    return perm_tables

PERM_TABLES = init_perm_tables()

# 2. MÔ HÌNH HÓA LINEAR MASKS THEO THUẬT TOÁN GỐC
def solve_linear_n_rounds(num_rounds=8):
    prob = pulp.LpProblem(f"Linear_Rubik4D_{num_rounds}R", pulp.LpMinimize)

    # Gamma_X[r][i]: Mask tuyến tính trước hộp S-box tại vòng r
    Gamma_X = [[pulp.LpVariable(f"LX_{r}_{i}", cat=pulp.LpBinary) for i in range(16)] for r in range(num_rounds + 1)]

    # Hàm mục tiêu: Tìm đường tuyến tính có tổng số active S-box ít nhất
    active_sboxes = []
    for r in range(num_rounds):
        active_sboxes.extend(Gamma_X[r])
    prob += pulp.lpSum(active_sboxes)

    # Ít nhất 1 byte mang mặt nạ không tầm thường
    prob += pulp.lpSum(Gamma_X[0]) >= 1

    for r in range(num_rounds):
        lut = PERM_TABLES[(r * 2) % 12]

        # 1. Qua S-box: Nếu byte có mask ở đầu vào thì có mask ở đầu ra (1-1 ở mức byte trừu tượng)
        # 2. Qua hoán vị SO(4): rot[i] = s_box_out[lut[i]]
        # Mặt nạ lan truyền theo vị trí dây: Gamma_Y[i] nhận mask từ Gamma_X[r][lut[i]]
        Gamma_Y = [pulp.LpVariable(f"LY_{r}_{i}", cat=pulp.LpBinary) for i in range(16)]
        for i in range(16):
            prob += Gamma_Y[i] == Gamma_X[r][lut[i]]

        # 3. Chuỗi 1-Pass ARX Ripple: 
        # for (int i = 0; i < 16; i++) { rot[(i+1)&15] ^= (rot[i] + 0x5A); }
        # Ở đây Z[s] là trạng thái mask trước bước s, Z[s+1] là trạng thái mask sau bước s
        Z = [[pulp.LpVariable(f"LZ_{r}_{s}_{i}", cat=pulp.LpBinary) for i in range(16)] for s in range(17)]
        for i in range(16):
            prob += Z[0][i] == Gamma_Y[i]

        for s in range(16):
            curr_idx = s
            next_idx = (s + 1) & 15

            # Các byte khác giữ nguyên
            for i in range(16):
                if i != curr_idx and i != next_idx:
                    prob += Z[s + 1][i] == Z[s][i]

            # Ràng buộc tuyến tính cho XOR và Branching:
            # rot[next] mới = rot[next] cũ ^ f(rot[curr] cũ)
            # - Mask tại rot[next] mới bắt buộc phải đi từ rot[next] cũ (bảo toàn parity)
            # - Hàm f(rot[curr]) nhận mask nếu có sự tương quan tuyến tính
            # => Nếu Z[s+1][next_idx] = 1: cả đầu vào next_idx và curr_idx đều bị kéo vào
            prob += Z[s][next_idx] >= Z[s + 1][next_idx]
            prob += Z[s][curr_idx] >= Z[s + 1][next_idx]
            
            # Nếu Z[s+1][next_idx] = 0 thì mask ở curr_idx truyền thẳng:
            prob += Z[s][curr_idx] >= Z[s + 1][curr_idx] - Z[s + 1][next_idx]
            
            # Không tự sinh mask từ 0:
            prob += Z[s][curr_idx] + Z[s][next_idx] <= 2 * Z[s + 1][curr_idx] + 2 * Z[s + 1][next_idx]

        # Kết thúc vòng r, chuyển mask sang vòng r+1
        for i in range(16):
            prob += Gamma_X[r + 1][i] == Z[16][i]

    solver = pulp.PULP_CBC_CMD(msg=False)
    prob.solve(solver)

    if prob.status == pulp.LpStatusOptimal:
        counts = [int(sum(pulp.value(Gamma_X[r][i]) for i in range(16))) for r in range(num_rounds)]
        return sum(counts), counts
    return None

if __name__ == "__main__":
    print("=" * 80)
    print(" QUÉT CẬN DƯỚI TOÁN HỌC ACTIVE S-BOXES: TUYẾN TÍNH (LINEAR CRYPTANALYSIS)")
    print("=" * 80)
    print(f"{'Cấu hình':<12} | {'Tổng Active':<12} | {'Phân bố từng vòng (R1 -> Rn)'}")
    print("-" * 80)

    for n in range(1, 9):
        total, dist = solve_linear_n_rounds(n)
        dist_str = " -> ".join(f"{c}" for c in dist)
        print(f"{n} Vòng{'':<6} | {total:<12} | {dist_str}")

    print("=" * 80)