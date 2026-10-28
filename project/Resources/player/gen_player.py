# ローポリ 上半身プレイヤー(ウェイクボーダー)生成スクリプト (OBJ + MTL + パレットテクスチャ)
# 座標系: Y-up / 右手系 (OBJ標準) / 正面 +Z / 単位メートル / 原点 = ボード底面の中心
# (形は腰 = y0 で組み立て、書き出し時に全体を BOARD_DROP だけ持ち上げる)
# 腕は T ポーズ (左腕 = +X, 右腕 = -X)。パーツごとに "o" を分けてあり、
# blender_rig.py がパーツ名と位置からウェイトを決める
import math, os, sys
from PIL import Image

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))

PALETTE = {
    "skin":   (236, 188, 150), "vest":   (255, 112, 24),  "vest2":  (36, 40, 48),
    "shirt":  (30, 96, 170),   "glove":  (40, 42, 46),    "helmet": (245, 245, 240),
    "helmet2": (30, 96, 170),  "eye":    (28, 26, 30),    "hair":   (60, 42, 30),
    "mouth":  (170, 90, 80),  "shorts": (46, 58, 84),  "board":  (22, 22, 26),
    "board2": (20, 170, 200), "boot":   (52, 54, 60),
}
NAMES = list(PALETTE)
CELL, GRID = 16, 4

def swatch_uv(name):
    k = NAMES.index(name)
    return (round((k % GRID + 0.5) / GRID, 6), round(1.0 - (k // GRID + 0.5) / GRID, 6))

def add(a, b): return (a[0]+b[0], a[1]+b[1], a[2]+b[2])
def sub(a, b): return (a[0]-b[0], a[1]-b[1], a[2]-b[2])
def mul(a, s): return (a[0]*s, a[1]*s, a[2]*s)
def cross(a, b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def dot(a, b): return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]
def norm(a):
    l = math.sqrt(dot(a, a)); return (a[0]/l, a[1]/l, a[2]/l)

parts = {}   # part名 -> list of (a, b, c, color, smooth)
cur = None
def part(name):
    global cur
    cur = parts.setdefault(name, [])

def tri(a, b, c, color, hint, smooth=True):
    n = cross(sub(b, a), sub(c, a))
    if dot(n, n) < 1e-14: return
    if dot(n, hint) < 0: b, c = c, b; n = mul(n, -1)
    cur.append((a, b, c, color, smooth, n))

def quad(a, b, c, d, color, hint, smooth=True):
    tri(a, b, c, color, hint, smooth); tri(a, c, d, color, hint, smooth)

def loft(axis, rings, sides, color_fn, cap0=True, cap1=True):
    """軸に沿った断面リングをつないだ筒。
    axis='y': rings=[(y, rx, rz, cx, cz)], axis='x': rings=[(x, ry, rz, cy, cz)]
    color_fn(ring_index, angle) -> 色名"""
    def ring_pts(r):
        t, a1, a2, c1, c2 = r
        pts = []
        for k in range(sides):
            ang = 2 * math.pi * k / sides
            if axis == "y": pts.append((c1 + a1 * math.sin(ang), t, c2 + a2 * math.cos(ang)))
            else:           pts.append((t, c1 + a1 * math.cos(ang), c2 + a2 * math.sin(ang)))
        return pts
    R = [ring_pts(r) for r in rings]
    for i in range(len(R) - 1):
        for k in range(sides):
            k2 = (k + 1) % sides
            a = R[i][k]
            c = (rings[i][3], a[1], rings[i][4]) if axis == "y" else (a[0], rings[i][3], rings[i][4])
            quad(R[i][k], R[i][k2], R[i+1][k2], R[i+1][k], color_fn(i, 2 * math.pi * (k + 0.5) / sides), sub(a, c))
    ax = (0, 1, 0) if axis == "y" else (1, 0, 0)
    for idx, sign, on in ((0, -1, cap0), (len(R) - 1, 1, cap1)):
        if not on: continue
        r = rings[idx]
        ctr = (r[3], r[0], r[4]) if axis == "y" else (r[0], r[3], r[4])
        for k in range(sides):
            tri(ctr, R[idx][k], R[idx][(k+1) % sides], color_fn(idx, 0.0), mul(ax, sign), smooth=False)

def ellipsoid(c, rx, ry, rz, sides, stacks, color_fn, phi0=0.0, phi1=math.pi, cap_bottom=False):
    """楕円体 (phi: 0=上の極, pi=下の極)。color_fn(point) -> 色名"""
    rows = []
    for j in range(stacks + 1):
        phi = phi0 + (phi1 - phi0) * j / stacks
        rows.append([(c[0] + rx * math.sin(phi) * math.sin(2 * math.pi * k / sides),
                      c[1] + ry * math.cos(phi),
                      c[2] + rz * math.sin(phi) * math.cos(2 * math.pi * k / sides)) for k in range(sides)])
    for j in range(stacks):
        for k in range(sides):
            k2 = (k + 1) % sides
            a, b, cc, d = rows[j][k], rows[j][k2], rows[j+1][k2], rows[j+1][k]
            mid = mul(add(a, cc), 0.5)
            quad(a, b, cc, d, color_fn(mid), sub(mid, c))
    if cap_bottom:
        y = rows[-1][0][1]; ctr = (c[0], y, c[2])
        for k in range(sides):
            tri(ctr, rows[-1][k], rows[-1][(k+1) % sides], color_fn(ctr), (0, -1, 0), smooth=False)

def box(x0, x1, y0, y1, z0, z1, color):
    P = lambda x, y, z: (x, y, z)
    for f, h in (((P(x0,y0,z0), P(x1,y0,z0), P(x1,y1,z0), P(x0,y1,z0)), (0,0,-1)),
                 ((P(x0,y0,z1), P(x1,y0,z1), P(x1,y1,z1), P(x0,y1,z1)), (0,0,1)),
                 ((P(x0,y0,z0), P(x0,y0,z1), P(x0,y1,z1), P(x0,y1,z0)), (-1,0,0)),
                 ((P(x1,y0,z0), P(x1,y0,z1), P(x1,y1,z1), P(x1,y1,z0)), (1,0,0)),
                 ((P(x0,y0,z0), P(x1,y0,z0), P(x1,y0,z1), P(x0,y0,z1)), (0,-1,0)),
                 ((P(x0,y1,z0), P(x1,y1,z0), P(x1,y1,z1), P(x0,y1,z1)), (0,1,0))):
        quad(*f, color, h, smooth=False)

# ================= 胴体 (ライフベスト) =================
# (y, 半幅rx, 半奥行rz, 中心x, 中心z)
TORSO = [(-0.118, 0.030, 0.022, 0, 0.000),   # 腰の下を丸く閉じる
         (-0.105, 0.085, 0.060, 0, 0.000),
         (-0.075, 0.125, 0.087, 0, 0.000),
         (-0.038, 0.145, 0.101, 0, 0.000),
         (0.00, 0.150, 0.105, 0, 0.000),
         (0.08, 0.152, 0.110, 0, 0.000),
         (0.16, 0.158, 0.115, 0, 0.005),
         (0.22, 0.165, 0.120, 0, 0.008),   # spine / chest の境目
         (0.30, 0.178, 0.126, 0, 0.010),
         (0.37, 0.185, 0.124, 0, 0.008),
         (0.42, 0.175, 0.112, 0, 0.000),   # 肩の高さ
         (0.46, 0.130, 0.090, 0, -0.005),
         (0.485, 0.065, 0.060, 0, -0.005)]
def torso_color(i, ang):
    y = TORSO[i][0]
    if y < 0: return "shorts"                       # 丸めた腰の下 (ショートパンツ)
    if y < 0.05: return "vest2"                     # 裾のベルト
    if abs(math.sin(ang)) < 0.12 and math.cos(ang) > 0: return "vest2"  # 正面のジッパー
    if y >= 0.42: return "shirt"                    # 肩まわりはシャツ
    return "vest"
part("torso"); loft("y", TORSO, 14, torso_color)
# 胸のバックル (左右に2本)
for yb in (0.14, 0.27):
    box(-0.10, 0.10, yb, yb + 0.022, 0.118 + 0.008 * (yb > 0.2), 0.134 + 0.008 * (yb > 0.2), "vest2")

# ================= 首・頭 =================
part("neck"); loft("y", [(0.44, 0.048, 0.050, 0, 0), (0.50, 0.046, 0.048, 0, 0.005), (0.57, 0.045, 0.046, 0, 0.01)],
                   10, lambda i, a: "skin")
HEAD_C = (0.0, 0.665, 0.012)
part("head")
ellipsoid(HEAD_C, 0.092, 0.112, 0.102, 12, 8,
          lambda p: "hair" if (p[2] < HEAD_C[2] - 0.03 and p[1] > HEAD_C[1] - 0.05) else "skin")
# 顔のパーツ (正面 +Z)
for s in (1, -1):
    box(s * 0.030 - 0.012, s * 0.030 + 0.012, 0.672, 0.692, 0.098, 0.112, "eye")
box(-0.010, 0.010, 0.640, 0.662, 0.104, 0.124, "skin")         # 鼻
box(-0.022, 0.022, 0.610, 0.617, 0.096, 0.108, "mouth")        # 口
for s in (1, -1):                                              # 耳
    box(s * 0.088 - 0.012, s * 0.088 + 0.012, 0.640, 0.690, -0.004, 0.024, "skin")
# ヘルメット (上半分を覆う殻 + ライン)
part("helmet")
ellipsoid((HEAD_C[0], HEAD_C[1] + 0.012, HEAD_C[2] - 0.004), 0.106, 0.122, 0.116, 12, 4,
          lambda p: "helmet2" if abs(p[0]) < 0.022 else "helmet", 0.0, math.pi * 0.52, cap_bottom=True)

# Mii風に頭を大きく (頭・顔・ヘルメットを首の付け根あたりを基準に拡大)
HEAD_SCALE, HEAD_LIFT = 1.6, 0.06
for pn in ("head", "helmet"):
    sc = lambda q: (HEAD_C[0] + (q[0] - HEAD_C[0]) * HEAD_SCALE,
                    HEAD_C[1] + (q[1] - HEAD_C[1]) * HEAD_SCALE + HEAD_LIFT,
                    HEAD_C[2] + (q[2] - HEAD_C[2]) * HEAD_SCALE)
    parts[pn] = [(sc(a), sc(b), sc(c), col, sm, n) for a, b, c, col, sm, n in parts[pn]]

# ================= 腕 (Tポーズ, 左右対称) =================
SHOULDER_Y = 0.415
def arm_rings(s):
    #   x,    ry(上下), rz(前後)
    R = [(0.13, 0.066, 0.066), (0.20, 0.072, 0.070), (0.28, 0.064, 0.062), (0.36, 0.056, 0.054),
         (0.41, 0.054, 0.052),                                            # 袖口
         (0.45, 0.050, 0.048), (0.49, 0.048, 0.046), (0.53, 0.047, 0.045),  # 肘 (曲げ用に3本)
         (0.60, 0.046, 0.044), (0.67, 0.040, 0.036), (0.73, 0.034, 0.030)]  # 手首
    return [(s * x, ry, rz, SHOULDER_Y, 0.0) for x, ry, rz in R]
for s, side in ((1, "L"), (-1, "R")):
    part("arm." + side)
    loft("x", arm_rings(s), 10, lambda i, a: "shirt" if i < 4 else "skin")
    # 手 (Miiのような丸い玉。指なし)
    part("hand." + side)
    ellipsoid((s * 0.765, SHOULDER_Y - 0.004, 0.004), 0.048, 0.046, 0.046, 10, 6, lambda p: "skin")


# ================= ウェイクボード (足元) =================
# 長手方向 = X (横向きに乗る)。腰から BOARD_DROP 下が板の底
BOARD_DROP = 0.85
BL, BW, BT, ROCKER = 1.40, 0.42, 0.03, 0.07
def board_half_width(u):          # u: -1..1 (板の端から端)
    a = abs(u)
    if a < 0.70: return BW / 2
    t = (a - 0.70) / 0.30
    return BW / 2 * math.sqrt(max(1 - t * t, 0.0)) * 0.92 + 0.012
def board_y(u):                   # ロッカー (両端が反り上がる)
    return -BOARD_DROP + BT / 2 + ROCKER * u * u
part("board")
NX, NZ = 20, 4
def bp(i, j, top):
    u = -1 + 2 * i / NX
    hw = board_half_width(u)
    return (u * BL / 2, board_y(u) + (BT / 2 if top else -BT / 2), hw * (-1 + 2 * j / NZ))
for i in range(NX):
    for j in range(NZ):
        quad(bp(i, j, True), bp(i+1, j, True), bp(i+1, j+1, True), bp(i, j+1, True), "board", (0, 1, 0), smooth=False)
        col = "board2" if (i // 3) % 2 == 0 else "helmet"
        quad(bp(i, j, False), bp(i+1, j, False), bp(i+1, j+1, False), bp(i, j+1, False), col, (0, -1, 0), smooth=False)
    for j in (0, NZ):   # 前後の縁
        quad(bp(i, j, True), bp(i+1, j, True), bp(i+1, j, False), bp(i, j, False), "helmet", (0, 0, -1 if j == 0 else 1), smooth=False)
for i in (0, NX):       # 両端の縁
    for j in range(NZ):
        quad(bp(i, j, True), bp(i, j+1, True), bp(i, j+1, False), bp(i, j, False), "helmet", (-1 if i == 0 else 1, 0, 0), smooth=False)
# フィン (両端の裏に2枚ずつ)
for su in (-1, 1):
    for sz in (-1, 1):
        u = su * 0.86; x = u * BL / 2; yb = board_y(u) - BT / 2; z = sz * 0.10
        a, b = (x - su * 0.05, yb, z), (x + su * 0.02, yb, z)
        c = (x + su * 0.01, yb - 0.035, z)
        tri(a, b, c, "board", (0, 0, sz), smooth=False); tri(a, c, b, "board", (0, 0, -sz), smooth=False)

# ================= 書き出し =================
os.makedirs(OUT, exist_ok=True)
r5 = lambda p: (round(p[0], 5), round(p[1] + BOARD_DROP, 5), round(p[2], 5))
rn = lambda p: tuple(round(x, 5) for x in p)
vi, ti, ni, V, T, N = {}, {}, {}, [], [], []
def idx(table, lst, key):
    if key not in table: table[key] = len(lst) + 1; lst.append(key)
    return table[key]
blocks, total = [], 0
for name, tl in parts.items():
    acc = {}
    for a, b, c, col, sm, n in tl:
        if sm:
            for p in (a, b, c): acc[r5(p)] = add(acc.get(r5(p), (0, 0, 0)), n)
    fl = []
    for a, b, c, col, sm, n in tl:
        uv = swatch_uv(col)
        fl.append([(idx(vi, V, r5(p)), idx(ti, T, uv), idx(ni, N, rn(norm(acc[r5(p)] if sm else n)))) for p in (a, b, c)])
    blocks.append((name, fl)); total += len(fl)
with open(os.path.join(OUT, "player_upper.obj"), "w", newline="\n") as f:
    f.write("# Low-poly upper-body player (wakeboarder)  Y-up / right-handed / faces +Z / meters / origin at board bottom center\n")
    f.write(f"# T-pose, left arm +X. tris: {total}\nmtllib player_upper.mtl\n")
    for v in V: f.write("v %.5f %.5f %.5f\n" % v)
    for t in T: f.write("vt %.6f %.6f\n" % t)
    for n in N: f.write("vn %.5f %.5f %.5f\n" % n)
    for name, fl in blocks:
        f.write(f"o {name}\nusemtl Player\n")
        for face in fl: f.write("f " + " ".join("%d/%d/%d" % x for x in face) + "\n")
with open(os.path.join(OUT, "player_upper.mtl"), "w", newline="\n") as f:
    f.write("newmtl Player\nKa 0 0 0\nKd 1 1 1\nKs 0 0 0\nd 1\nillum 1\nmap_Kd player_palette.png\n")
img = Image.new("RGB", (CELL*GRID, CELL*GRID), (255, 0, 255))
for k, name in enumerate(NAMES):
    img.paste(PALETTE[name], ((k % GRID)*CELL, (k // GRID)*CELL, (k % GRID + 1)*CELL, (k // GRID + 1)*CELL))
img.save(os.path.join(OUT, "player_palette.png"))
print("tris", total, {n: len(fl) for n, fl in blocks})
