# ローポリ小島 生成スクリプト (OBJ + MTL + パレットテクスチャ, 5種類)
# 座標系: Y-up / 右手系 (OBJ標準) / 単位メートル / 原点 = 喫水線(y=0)上の島の中心
# 水面下は底が丸く閉じた「浮き島」の形 (潜っても破綻しない閉じたメッシュ)
# 使い方: python gen_islands.py [出力フォルダ]
import math, os, sys, json, random
from PIL import Image

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))

PALETTE = {
    "sand":  (222, 200, 150), "wet":   (168, 150, 112), "grass": (98, 150, 70),
    "grass2": (72, 124, 58),  "rock":  (118, 114, 108), "rock2": (88, 86, 84),
    "trunk": (120, 88, 58),   "leaf":  (64, 136, 60),   "deep":  (58, 62, 70),
    "flower": (232, 210, 90),
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
def lerp(a, b, t): return a + (b - a) * t

# ---- シード付き 1D/2D バリューノイズ ----
class Noise:
    def __init__(self, seed):
        r = random.Random(seed); self.t = [r.random() for _ in range(512)]
    def h(self, i, j=0): return self.t[(i * 73 + j * 151) & 511]
    def n1(self, x, period):  # 周期付き(角度用)
        i = math.floor(x); f = x - i; f = f*f*(3-2*f)
        return lerp(self.h(i % period), self.h((i+1) % period), f)
    def n2(self, x, y):
        i, j = math.floor(x), math.floor(y); fx, fy = x-i, y-j
        fx, fy = fx*fx*(3-2*fx), fy*fy*(3-2*fy)
        a = lerp(self.h(i, j), self.h(i+1, j), fx); b = lerp(self.h(i, j+1), self.h(i+1, j+1), fx)
        return lerp(a, b, fy)

class Mesh:
    def __init__(self): self.tris = []
    def tri(self, a, b, c, color, hint, two_sided=False):
        n = cross(sub(b, a), sub(c, a))
        if dot(n, n) < 1e-12: return
        if dot(n, hint) < 0: b, c = c, b; n = mul(n, -1)
        self.tris.append((a, b, c, norm(n), color))
        if two_sided: self.tris.append((a, c, b, norm(mul(n, -1)), color))
    def quad(self, a, b, c, d, color, hint, two_sided=False):
        self.tri(a, b, c, color, hint, two_sided); self.tri(a, c, d, color, hint, two_sided)

    def write(self, name):
        vi, ti, ni, V, T, N, F = {}, {}, {}, [], [], [], []
        def idx(table, lst, key):
            if key not in table: table[key] = len(lst) + 1; lst.append(key)
            return table[key]
        r = lambda p, d=4: tuple(round(x, d) for x in p)
        for a, b, c, n, col in self.tris:
            uv = swatch_uv(col)
            F.append([(idx(vi, V, r(p)), idx(ti, T, uv), idx(ni, N, r(n))) for p in (a, b, c)])
        with open(os.path.join(OUT, name + ".obj"), "w", newline="\n") as f:
            f.write(f"# {name}  Y-up / right-handed / meters / origin at waterline center\n")
            f.write(f"# tris: {len(F)}\nmtllib islands.mtl\no {name}\nusemtl Island\ns off\n")
            for v in V: f.write("v %.4f %.4f %.4f\n" % v)
            for t in T: f.write("vt %.6f %.6f\n" % t)
            for n in N: f.write("vn %.4f %.4f %.4f\n" % n)
            for face in F: f.write("f " + " ".join("%d/%d/%d" % x for x in face) + "\n")
        return len(F)

# ---- 地形本体: 放射状グリッド ----
def terrain(m, rng, noise, radius, height_fn, segs=28, rings=10, rocky=0.0, depth_ratio=0.85):
    """radius: 喫水線での平均半径, height_fn(t)->高さ (t=0中心, t=1喫水線)
    水面より上は放射状グリッド、水面下は丸く閉じた底 (浮き島の形)"""
    ang_off = [noise.n1(k * 5.0 / segs, 5) for k in range(segs)]   # 外周のでこぼこ
    rr_of = lambda k: radius * (0.75 + 0.5 * ang_off[k % segs])
    def P(i, k):
        a = 2 * math.pi * k / segs
        t = i / rings
        rr = rr_of(k)
        x, z = math.cos(a) * rr * t, math.sin(a) * rr * t
        y = height_fn(t) + (noise.n2(x * 0.35 + 7, z * 0.35 + 3) - 0.5) * height_fn(0) * 0.25 * min(1.0, 2.5 * (1 - t))
        if i == rings: y = 0.0
        return (x, y, z)
    grid = [[P(i, k) for k in range(segs)] for i in range(rings + 1)]
    top = max(p[1] for row in grid for p in row)
    def color_of(a, b, c):
        y = (a[1] + b[1] + c[1]) / 3
        n = norm(cross(sub(b, a), sub(c, a))); slope = abs(n[1])
        if y < 0.12: return "wet"
        if slope < 0.62 - 0.2 * rocky or (rocky > 0.5 and y > top * 0.55): return "rock" if rng.random() < 0.6 else "rock2"
        if y < 0.55: return "sand"
        return "grass" if rng.random() < 0.65 else "grass2"
    center = (0.0, height_fn(0), 0.0)
    for k in range(segs):  # 中心の扇
        a, b = grid[1][k], grid[1][(k+1) % segs]
        m.tri(center, a, b, color_of(center, a, b), (0, 1, 0))
    for i in range(1, len(grid) - 1):
        for k in range(segs):
            k2 = (k + 1) % segs
            a, b, c, d = grid[i][k], grid[i][k2], grid[i+1][k2], grid[i+1][k]
            # 向きは「真上から見た(平らにした)形」で決める。凸凹の斜面でも裏返らない
            for t3 in ((a, b, c), (a, c, d)):
                flat = cross(sub((t3[1][0], 0, t3[1][2]), (t3[0][0], 0, t3[0][2])),
                             sub((t3[2][0], 0, t3[2][2]), (t3[0][0], 0, t3[0][2])))
                p0, p1, p2 = t3 if flat[1] > 0 else (t3[0], t3[2], t3[1])
                m.tri(p0, p1, p2, color_of(p0, p1, p2), cross(sub(p1, p0), sub(p2, p0)))

    # ---- 水面下: 少し張り出してから丸く絞った底 ----
    depth = radius * depth_ratio
    UR = 8
    under = [grid[-1]]
    for j in range(1, UR + 1):
        phi = (math.pi / 2) * j / (UR + 1)
        row = []
        for k in range(segs):
            a = 2 * math.pi * k / segs
            jit = 1 + 0.18 * (noise.n2(k * 0.9 + 31, j * 1.3 + 5) - 0.5)
            lip = 1 + 0.10 * math.sin(phi * 2) ** 2          # 水面直下の張り出し
            r = rr_of(k) * lip * math.cos(phi) ** 0.9 * jit
            y = -depth * math.sin(phi) ** 1.3 * (1 + 0.12 * (noise.n2(k * 1.1 + 9, j * 0.7) - 0.5))
            row.append((math.cos(a) * r, y, math.sin(a) * r))
        under.append(row)
    bottom = (radius * 0.08, -depth * 1.05, -radius * 0.05)
    mid = (0.0, -depth * 0.4, 0.0)
    def under_color(p0, p1, p2):
        y = (p0[1] + p1[1] + p2[1]) / 3
        if y > -0.4: return "wet"
        f = -y / depth
        if f < 0.4: return "rock" if rng.random() < 0.65 else "rock2"
        return "rock2" if rng.random() < 0.55 else "deep"
    for j in range(len(under) - 1):
        for k in range(segs):
            k2 = (k + 1) % segs
            a, b, c, d = under[j][k], under[j][k2], under[j+1][k2], under[j+1][k]
            hint = sub(mul(add(a, c), 0.5), mid)
            m.tri(a, b, c, under_color(a, b, c), hint); m.tri(a, c, d, under_color(a, c, d), hint)
    for k in range(segs):
        a, b = under[-1][k], under[-1][(k+1) % segs]
        m.tri(bottom, a, b, under_color(bottom, a, b), (0, -1, 0))
    # 底から垂れ下がる岩のトゲ (浮き島らしさ)
    for _ in range(3):
        k = rng.randrange(segs); j = rng.randrange(UR // 2, UR)
        base = under[j][k]; L = depth * rng.uniform(0.25, 0.45); w = radius * rng.uniform(0.07, 0.12)
        ring5 = [(base[0] * 0.92 + w * math.cos(2 * math.pi * q / 5), base[1] + w * 0.3, base[2] * 0.92 + w * math.sin(2 * math.pi * q / 5)) for q in range(5)]
        tip = (base[0] * 0.85, base[1] - L, base[2] * 0.85)
        ctr = (base[0] * 0.92, base[1], base[2] * 0.92)
        for q in range(5):
            a, b = ring5[q], ring5[(q + 1) % 5]
            m.tri(a, b, tip, "rock2", sub(mul(add(a, b), 0.5), (ctr[0], tip[1], ctr[2])))
    def surface_y(x, z):  # 小物を地面に置くための高さ(近似: 一番近い頂点)
        best = min((p for row in grid for p in row), key=lambda p: (p[0]-x)**2 + (p[2]-z)**2)
        return best[1]
    return surface_y, top

# ---- 小物 ----
def rock(m, rng, c, size, color="rock"):
    """八面体を1回分割して歪ませた岩"""
    base = [(1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1)]
    faces = [(0,2,4),(4,2,1),(1,2,5),(5,2,0),(0,4,3),(4,1,3),(1,5,3),(5,0,3)]
    verts = list(base); cache = {}
    def mid(i, j):
        key = tuple(sorted((i, j)))
        if key not in cache: verts.append(norm(mul(add(verts[i], verts[j]), 0.5))); cache[key] = len(verts) - 1
        return cache[key]
    fs = []
    for a, b, cc in faces:
        ab, bc, ca = mid(a, b), mid(b, cc), mid(cc, a)
        fs += [(a, ab, ca), (ab, b, bc), (ca, bc, cc), (ab, bc, ca)]
    sx, sy, sz = size * rng.uniform(0.8, 1.3), size * rng.uniform(0.6, 1.0), size * rng.uniform(0.8, 1.3)
    jit = [rng.uniform(0.8, 1.15) for _ in verts]
    P = [(c[0] + v[0]*sx*jit[i], c[1] + v[1]*sy*jit[i], c[2] + v[2]*sz*jit[i]) for i, v in enumerate(verts)]
    for a, b, cc in fs:
        ctr = mul(add(add(P[a], P[b]), P[cc]), 1/3)
        m.tri(P[a], P[b], P[cc], color if rng.random() < 0.7 else "rock2", sub(ctr, c))

def palm(m, rng, base, height):
    lean = rng.uniform(0, 2 * math.pi); bend = rng.uniform(0.15, 0.3) * height
    segs, sides, r0 = 5, 5, 0.18 * height / 5
    def spine(t):
        return (base[0] + math.cos(lean) * bend * t * t, base[1] + height * t, base[2] + math.sin(lean) * bend * t * t)
    rings = []
    for s in range(segs + 1):
        t = s / segs; c = spine(t); r = r0 * (1 - 0.4 * t)
        rings.append([(c[0] + r*math.cos(2*math.pi*k/sides), c[1], c[2] + r*math.sin(2*math.pi*k/sides)) for k in range(sides)])
    for s in range(segs):
        c = spine((s + 0.5) / segs)
        for k in range(sides):
            k2 = (k + 1) % sides
            a = rings[s][k]
            m.quad(rings[s][k], rings[s][k2], rings[s+1][k2], rings[s+1][k], "trunk", (a[0]-c[0], 0, a[2]-c[2]))
    top = spine(1.0)
    for f in range(7):  # 葉 (両面)
        a = 2 * math.pi * f / 7 + rng.uniform(-0.2, 0.2)
        L = height * rng.uniform(0.45, 0.6); w = L * 0.22
        d = (math.cos(a), 0, math.sin(a)); side = (-d[2], 0, d[0])
        mid_p = add(top, (d[0]*L*0.5, 0.25*height*0.1, d[2]*L*0.5))
        tip = add(top, (d[0]*L, -L*0.45, d[2]*L))
        l, r = add(mid_p, mul(side, w)), add(mid_p, mul(side, -w))
        m.tri(top, l, mid_p, "leaf", (0, 1, 0), True); m.tri(top, mid_p, r, "leaf", (0, 1, 0), True)
        m.tri(l, tip, mid_p, "leaf", (0, 1, 0), True); m.tri(mid_p, tip, r, "leaf", (0, 1, 0), True)

def bush(m, rng, c, size):
    rock(m, rng, add(c, (0, size * 0.5, 0)), size, "grass2")

# ---- 5種類の島 ----
def scatter(rng, radius, n, rmin=0.0, rmax=0.6):
    pts = []
    for _ in range(n):
        a = rng.uniform(0, 2*math.pi); d = radius * math.sqrt(rng.uniform(rmin**2, rmax**2))
        pts.append((math.cos(a) * d, math.sin(a) * d))
    return pts

ISLANDS = []
def build(name, seed, radius, hfn, rocky=0.0, palms=0, bushes=0, rocks=0, rock_size=1.0, rock_ring=(0.3, 0.9)):
    rng = random.Random(seed); m = Mesh()
    ysurf, top = terrain(m, rng, Noise(seed), radius, hfn, rocky=rocky)
    for x, z in scatter(rng, radius, palms, 0.0, 0.45):
        palm(m, rng, (x, ysurf(x, z) - 0.1, z), rng.uniform(3.5, 5.0))
    for x, z in scatter(rng, radius, bushes, 0.1, 0.6):
        bush(m, rng, (x, ysurf(x, z) - 0.2, z), rng.uniform(0.5, 0.9))
    for x, z in scatter(rng, radius, rocks, *rock_ring):
        rock(m, rng, (x, max(ysurf(x, z), -0.3), z), rock_size * rng.uniform(0.6, 1.2))
    n = m.write(name)
    ymax = max(max(t[0][1], t[1][1], t[2][1]) for t in m.tris)
    rmax = max(math.hypot(p[0], p[2]) for t in m.tris for p in t[:3] if p[1] >= -0.01)
    ymin = min(min(t[0][1], t[1][1], t[2][1]) for t in m.tris)
    ISLANDS.append(dict(name=name, tris=n, waterline_radius=round(rmax, 2), height=round(ymax, 2), bottom=round(ymin, 2),
                        max_radius=round(max(math.hypot(p[0], p[2]) for t in m.tris for p in t[:3]), 2)))

smooth_hill = lambda h, p=1.6: (lambda t: h * (1 - t**p) if t < 1 else 0)
build("island_sandbar", 11, 5.0,  smooth_hill(0.9, 2.5), palms=1, bushes=2, rocks=2, rock_size=0.5)
build("island_palm",    23, 8.0,  smooth_hill(2.8, 2.0), palms=3, bushes=5, rocks=3, rock_size=0.7)
build("island_hill",    37, 12.0, smooth_hill(7.5, 1.3), rocky=0.3, palms=2, bushes=8, rocks=5, rock_size=1.0)
build("island_crag",    41, 6.0,  smooth_hill(7.0, 0.9), rocky=1.0, rocks=6, rock_size=1.4, rock_ring=(0.6, 1.0))
build("island_mesa",    53, 9.0,  lambda t: 4.5 if t < 0.55 else 4.5 * (1 - ((t - 0.55) / 0.45)**0.6),
      rocky=0.6, palms=2, bushes=4, rocks=4, rock_size=1.1, rock_ring=(0.75, 1.0))

with open(os.path.join(OUT, "islands.mtl"), "w", newline="\n") as f:
    f.write("newmtl Island\nKa 0 0 0\nKd 1 1 1\nKs 0 0 0\nd 1\nillum 1\nmap_Kd islands_palette.png\n")
img = Image.new("RGB", (CELL*GRID, CELL*GRID), (255, 0, 255))
for k, name in enumerate(NAMES):
    img.paste(PALETTE[name], ((k % GRID)*CELL, (k // GRID)*CELL, (k % GRID + 1)*CELL, (k // GRID + 1)*CELL))
img.save(os.path.join(OUT, "islands_palette.png"))
with open(os.path.join(OUT, "islands.json"), "w") as f: json.dump(ISLANDS, f, indent=2)
for i in ISLANDS: print(i)
