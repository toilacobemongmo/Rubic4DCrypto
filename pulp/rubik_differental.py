import pulp

# 1. KHỞI TẠO BẢNG HOÁN VỊ
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

# 2. MÔ HÌNH HÓA MILP 8 VÒNG
def solve_n_rounds(num_rounds=8):
    prob = pulp.LpProblem(f"Rubik4D_{num_rounds}R", pulp.LpMinimize)

    # X[r][i]: Trạng thái active byte thứ i ở đầu Round r
    X = [[pulp.LpVariable(f"X_{r}_{i}", cat=pulp.LpBinary) for i in range(16)] for r in range(num_rounds + 1)]

    # Hàm mục tiêu: Tối thiểu hóa tổng active S-boxes
    active_sboxes = []
    for r in range(num_rounds):
        active_sboxes.extend(X[r])
    prob += pulp.lpSum(active_sboxes)

    # Ràng buộc đầu vào không tầm thường
    prob += pulp.lpSum(X[0]) >= 1

    for r in range(num_rounds):
        lut = PERM_TABLES[(r * 2) % 12]

        # Tầng SO(4)
        Y = [pulp.LpVariable(f"Y_{r}_{i}", cat=pulp.LpBinary) for i in range(16)]
        for i in range(16):
            prob += Y[i] == X[r][lut[i]]

        # Tầng 1-pass ARX Ripple (16 bước)
        Z = [[pulp.LpVariable(f"Z_{r}_{s}_{i}", cat=pulp.LpBinary) for i in range(16)] for s in range(17)]
        for i in range(16):
            prob += Z[0][i] == Y[i]

        for s in range(16):
            curr_idx = s
            next_idx = (s + 1) & 15
            for i in range(16):
                if i != next_idx:
                    prob += Z[s + 1][i] == Z[s][i]

            prob += Z[s + 1][next_idx] >= Z[s][curr_idx] - Z[s][next_idx]
            prob += Z[s + 1][next_idx] >= Z[s][next_idx] - Z[s][curr_idx]
            prob += Z[s + 1][next_idx] <= Z[s][curr_idx] + Z[s][next_idx]

        for i in range(16):
            prob += X[r + 1][i] == Z[16][i]

    solver = pulp.PULP_CBC_CMD(msg=False)
    prob.solve(solver)

    if prob.status == pulp.LpStatusOptimal:
        counts = [int(sum(pulp.value(X[r][i]) for i in range(16))) for r in range(num_rounds)]
        return sum(counts), counts
    return None

if __name__ == "__main__":
    print("=" * 80)
    print(" QUÉT CẬN DƯỚI TOÁN HỌC ACTIVE S-BOXES TỪ 1 ĐẾN 8 VÒNG (MILP / CBC)")
    print("=" * 80)
    print(f"{'Cấu hình':<12} | {'Tổng Active':<12} | {'Phân bố từng vòng (R1 -> Rn)'}")
    print("-" * 80)

    for n in range(1, 9):
        total, dist = solve_n_rounds(n)
        dist_str = " -> ".join(f"{c}" for c in dist)
        print(f"{n} Vòng{'':<6} | {total:<12} | {dist_str}")

    print("=" * 80)