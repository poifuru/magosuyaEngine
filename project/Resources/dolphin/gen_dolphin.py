# ローポリ イルカ生成スクリプト (OBJ + MTL + パレットテクスチャ)
# 座標系: Y-up / 右手系 (OBJ標準) / 頭 +Z / 単位メートル / 原点 = 胴体の中心
# パーツごとに "o" を分けてあり、blender_rig_dolphin.py が骨とウェイトを付ける
import math, os, sys
from PIL import Image

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))

PALETTE = {
    "back":  (92, 124, 150),  "belly": (222, 228, 232), "fin":  (78, 106, 130),
    "eye":   (20, 22, 28),    "mouth": (60, 70, 80),    "shine": (250, 250, 255),
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

parts, cur = {}, None
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

def loft_z(rings, sides, color_fn):
    """Z軸に沿った筒。rings=[(z, rx, ry, cx, cy)]。両端は中心へ閉じる"""
    R = [[(cx + rx*math.cos(2*math.pi*k/sides), cy + ry*math.sin(2*math.pi*k/sides), z) for k in range(sides)]
         for z, rx, ry, cx, cy in rings]
    for i in range(len(R) - 1):
        for k in range(sides):
            k2 = (k + 1) % sides
            ang = 2*math.pi*(k + 0.5)/sides
            a = R[i][k]; z, rx, ry, cx, cy = rings[i]
            quad(R[i][k], R[i][k2], R[i+1][k2], R[i+1][k], color_fn(i, ang), (a[0]-cx, a[1]-cy, 0))
    for idx, sgn in ((0, -1), (len(R)-1, 1)):
        z, rx, ry, cx, cy = rings[idx]
        for k in range(sides):
            tri((cx, cy, z + sgn*0.01), R[idx][k], R[idx][(k+1) % sides], color_fn(idx, math.pi/2), (0, 0, sgn))

def plate(pts, thick, color):
    """平らな多角形 (ひれ) に厚みを付ける。pts は星形(重心から全点が見える)であること"""
    c = mul(pts[0], 0)
    for p in pts: c = add(c, p)
    c = mul(c, 1/len(pts))
    n = (0, 0, 0)
    for i in range(len(pts)):
        n = add(n, cross(sub(pts[i], c), sub(pts[(i+1) % len(pts)], c)))
    n = norm(n)
    top = [add(p, mul(n, thick/2)) for p in pts]; bot = [add(p, mul(n, -thick/2)) for p in pts]
    # 縁は薄く: 外周の点は厚みを半分に
    top = [add(p, mul(n, -thick/4)) for p in top]; bot = [add(p, mul(n, thick/4)) for p in bot]
    ct, cb = add(c, mul(n, thick/2)), add(c, mul(n, -thick/2))
    for i in range(len(pts)):
        j = (i+1) % len(pts)
        tri(ct, top[i], top[j], color, n, smooth=False)
        tri(cb, bot[i], bot[j], color, mul(n, -1), smooth=False)
        mid = mul(add(pts[i], pts[j]), 0.5)
        quad(top[i], top[j], bot[j], bot[i], color, sub(mid, c), smooth=False)

def blob(c, r, sides, stacks, color, scale=(1, 1, 1)):
    rows = [[(c[0] + r*scale[0]*math.sin(math.pi*j/stacks)*math.cos(2*math.pi*k/sides),
              c[1] + r*scale[1]*math.cos(math.pi*j/stacks),
              c[2] + r*scale[2]*math.sin(math.pi*j/stacks)*math.sin(2*math.pi*k/sides)) for k in range(sides)]
            for j in range(stacks + 1)]
    for j in range(stacks):
        for k in range(sides):
            k2 = (k + 1) % sides
            a = rows[j][k]
            quad(rows[j][k], rows[j][k2], rows[j+1][k2], rows[j+1][k], color, sub(rows[j+1][k2], c) if j == 0 else sub(a, c))

# ================= 胴体 (尾 → くちばし) =================
#        z,     rx,    ry,    cx, cy
BODY = [(-1.24, 0.030, 0.060, 0, 0.030),
        (-1.15, 0.050, 0.085, 0, 0.030),   # 尾の付け根 (細い)
        (-0.95, 0.090, 0.140, 0, 0.035),
        (-0.70, 0.150, 0.195, 0, 0.040),
        (-0.40, 0.210, 0.245, 0, 0.040),
        (-0.10, 0.245, 0.280, 0, 0.030),
        ( 0.20, 0.240, 0.270, 0, 0.020),
        ( 0.45, 0.200, 0.230, 0, 0.020),
        ( 0.62, 0.150, 0.180, 0, 0.010),   # 頭 (メロン)
        ( 0.74, 0.100, 0.115, 0, -0.030),
        ( 0.80, 0.062, 0.060, 0, -0.065),  # くちばしの付け根
        ( 0.95, 0.050, 0.045, 0, -0.075),
        ( 1.08, 0.030, 0.028, 0, -0.078)]  # くちばしの先
def body_color(i, ang):
    s = math.sin(ang)
    if i >= 10: return "back" if s > 0.2 else "belly"          # くちばしは下半分が白
    return "belly" if s < -0.30 + 0.25 * (i <= 2) else "back"
part("body"); loft_z(BODY, 16, body_color)

# 背びれ (後ろに反った三角)
part("dorsal")
plate([(0, 0.255, 0.12), (0, 0.42, -0.10), (0, 0.58, -0.30), (0, 0.46, -0.24), (0, 0.27, -0.34)], 0.045, "fin")

# 胸びれ (左右、下後ろ向き)
for s, side in ((1, "L"), (-1, "R")):
    part("pec." + side)
    plate([(s*0.19, -0.10, 0.36), (s*0.36, -0.22, 0.20), (s*0.46, -0.30, 0.02),
           (s*0.36, -0.24, 0.08), (s*0.19, -0.14, 0.16)], 0.035, "fin")

# 尾びれ (水平。真ん中に切れ込み)
part("fluke")
plate([(0, 0.03, -1.18), (0.40, 0.03, -1.40), (0.36, 0.03, -1.52), (0.10, 0.03, -1.44), (0, 0.03, -1.48),
       (-0.10, 0.03, -1.44), (-0.36, 0.03, -1.52), (-0.40, 0.03, -1.40)], 0.04, "fin")

# 顔 (目・ハイライト・口・噴気孔)
part("face")
for s in (1, -1):
    blob((s*0.142, 0.020, 0.60), 0.026, 8, 4, "eye")
    blob((s*0.155, 0.032, 0.612), 0.008, 6, 3, "shine")
    # 口のライン (くちばしの付け根から目の下へ少し上がる、笑った口)
    pts = [(1.04, -0.082), (0.92, -0.084), (0.80, -0.078), (0.70, -0.060), (0.63, -0.035)]
    for (z0, y0), (z1, y1) in zip(pts, pts[1:]):
        def surf_x(z, y):   # 胴体の楕円上のx (少し外側)
            for i in range(len(BODY) - 1):
                if BODY[i][0] <= z <= BODY[i+1][0]:
                    t = (z - BODY[i][0]) / (BODY[i+1][0] - BODY[i][0])
                    rx = BODY[i][1] + (BODY[i+1][1] - BODY[i][1]) * t
                    ry = BODY[i][2] + (BODY[i+1][2] - BODY[i][2]) * t
                    cy = BODY[i][4] + (BODY[i+1][4] - BODY[i][4]) * t
                    v = max(-1.0, min(1.0, (y - cy) / ry))
                    return rx * math.sqrt(1 - v*v) + 0.004
            return 0.0
        a = (s*surf_x(z0, y0), y0, z0); b = (s*surf_x(z1, y1), y1, z1)
        quad(a, b, (b[0], b[1] - 0.010, b[2]), (a[0], a[1] - 0.010, a[2]), "mouth", (s, 0, 0), smooth=False)
blob((0, 0.245, 0.36), 0.022, 8, 3, "mouth", scale=(1, 0.3, 1.4))   # 噴気孔

# ================= 書き出し =================
os.makedirs(OUT, exist_ok=True)
r5 = lambda p: tuple(round(x, 5) for x in p)
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
        fl.append([(idx(vi, V, r5(p)), idx(ti, T, uv), idx(ni, N, r5(norm(acc[r5(p)] if sm else n)))) for p in (a, b, c)])
    blocks.append((name, fl)); total += len(fl)
with open(os.path.join(OUT, "dolphin.obj"), "w", newline="\n") as f:
    f.write(f"# Low-poly dolphin  Y-up / right-handed / head +Z / meters / origin at body center\n# tris: {total}\nmtllib dolphin.mtl\n")
    for v in V: f.write("v %.5f %.5f %.5f\n" % v)
    for t in T: f.write("vt %.6f %.6f\n" % t)
    for n in N: f.write("vn %.5f %.5f %.5f\n" % n)
    for name, fl in blocks:
        f.write(f"o {name}\nusemtl Dolphin\n")
        for face in fl: f.write("f " + " ".join("%d/%d/%d" % x for x in face) + "\n")
with open(os.path.join(OUT, "dolphin.mtl"), "w", newline="\n") as f:
    f.write("newmtl Dolphin\nKa 0 0 0\nKd 1 1 1\nKs 0.3 0.3 0.3\nd 1\nillum 2\nmap_Kd dolphin_palette.png\n")
img = Image.new("RGB", (CELL*GRID, CELL*GRID), (255, 0, 255))
for k, name in enumerate(NAMES):
    img.paste(PALETTE[name], ((k % GRID)*CELL, (k // GRID)*CELL, (k % GRID + 1)*CELL, (k // GRID + 1)*CELL))
img.save(os.path.join(OUT, "dolphin_palette.png"))
print("tris", total, {n: len(fl) for n, fl in blocks})
