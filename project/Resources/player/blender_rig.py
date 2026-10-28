# Blender 用: player_upper.obj を読み込み、アーマチュア(骨)とウェイトを設定して
# player_upper_rigged.blend と player_upper.gltf を書き出すスクリプト
#
# 使い方 (どちらでも可):
#   1) コマンドライン: blender -b -P blender_rig.py -- <player_upper.obj> <出力フォルダ>
#   2) Blender の Scripting タブで開いて実行 (OBJ と同じフォルダにこのスクリプトを置く)
#
# 骨: board(ルート), spine, chest, neck, head, upper_arm.L/R, forearm.L/R, hand.L/R (計11本)
# board を動かすと全身が、spine を動かすと上半身だけが動く (しゃがみ・溜めは spine を下げる)
# 左右は Blender の慣例どおり .L = キャラの左 = +X
# サンプルとして、両手でロープのハンドルを持つポーズの "RopeHold" アクションを1つ付ける
import bpy, bmesh, math, os, sys
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
here = os.path.dirname(bpy.data.filepath or __file__) if not argv else None
OBJ = argv[0] if argv else os.path.join(here, "player_upper.obj")
OUT = argv[1] if len(argv) > 1 else os.path.dirname(os.path.abspath(OBJ))

# 骨やウェイトの数値は「腰 = y0」の座標で書いてある。OBJ は全体を BOARD_DROP 持ち上げて
# ボード底面が原点になっているので、その分をここで足し引きする (gen_player.py と同じ値)
BOARD_DROP = 0.85
def B(p):
    """腰基準の座標 (Y-up, 正面+Z) → Blender座標 (Z-up, 正面-Y, 原点=ボード底面)"""
    return Vector((p[0], -p[2], p[1] + BOARD_DROP))
def O(v):
    """Blender座標 → 腰基準の座標"""
    return (v.x, v.z - BOARD_DROP, -v.y)

# ---- シーンを空にして OBJ を読み込む ----
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.wm.obj_import(filepath=OBJ, forward_axis='NEGATIVE_Z', up_axis='Y')
meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']

# ---- 骨 (OBJ座標で定義) ----
SY = 0.415
BONES = {  # name: (head, tail, parent, connected)
    "board":   ((0, -BOARD_DROP, 0), (0, -BOARD_DROP + 0.25, 0), None, False),   # ルート (ボード底面の中心)
    "spine":   ((0, 0.00, 0), (0, 0.22, 0), "board", False),
    "chest":   ((0, 0.22, 0), (0, 0.44, 0), "spine", True),
    "neck":    ((0, 0.44, 0), (0, 0.57, 0), "chest", True),
    "head":    ((0, 0.57, 0), (0, 0.97, 0), "neck", True),
}
for s, side in ((1, "L"), (-1, "R")):
    BONES[f"upper_arm.{side}"] = ((s*0.16, SY, 0), (s*0.49, SY, 0), "chest", False)
    BONES[f"forearm.{side}"]   = ((s*0.49, SY, 0), (s*0.725, SY, 0), f"upper_arm.{side}", True)
    BONES[f"hand.{side}"]      = ((s*0.725, SY, 0), (s*0.815, SY, 0), f"forearm.{side}", True)

arm_data = bpy.data.armatures.new("PlayerRig")
rig = bpy.data.objects.new("PlayerRig", arm_data)
bpy.context.scene.collection.objects.link(rig)
bpy.context.view_layer.objects.active = rig
bpy.ops.object.mode_set(mode='EDIT')
for name, (h, t, parent, conn) in BONES.items():
    eb = arm_data.edit_bones.new(name)
    eb.head, eb.tail = B(h), B(t)
    eb.roll = 0.0
for name, (h, t, parent, conn) in BONES.items():
    if parent:
        eb = arm_data.edit_bones[name]
        eb.parent = arm_data.edit_bones[parent]; eb.use_connect = conn
bpy.ops.object.mode_set(mode='OBJECT')

# ---- ウェイト (パーツ名と位置で決める。関節付近だけ2本の骨にブレンド) ----
def clamp01(x): return max(0.0, min(1.0, x))
def weights_for(part, p):
    x, y, z = p
    if part == "torso":
        t = clamp01((y - 0.18) / 0.08); return {"spine": 1 - t, "chest": t}
    if part == "neck":
        t = clamp01((y - 0.44) / 0.05); u = clamp01((y - 0.54) / 0.03)
        return {"chest": 1 - t, "neck": t * (1 - u), "head": t * u}
    if part in ("head", "helmet"):
        return {"head": 1.0}
    if part == "board":
        return {"board": 1.0}
    side = part.split(".")[1]
    if part.startswith("arm."):
        ax = abs(x)
        t = clamp01((ax - 0.45) / 0.08); u = clamp01((ax - 0.69) / 0.04)
        return {f"upper_arm.{side}": 1 - t, f"forearm.{side}": t * (1 - u), f"hand.{side}": t * u}
    if part.startswith("hand."):
        return {f"hand.{side}": 1.0}
    raise ValueError(part)

for ob in meshes:
    part = ob.name.split(".0")[0]          # Blender が付ける ".001" を除く
    if part not in ("torso", "neck", "head", "helmet", "board") and not part.startswith(("arm.", "hand.")):
        raise RuntimeError("unknown part " + ob.name)
    groups = {}
    for v in ob.data.vertices:
        w = weights_for(part, O(ob.matrix_world @ v.co))
        for bone, val in w.items():
            if val <= 1e-4: continue
            if bone not in groups: groups[bone] = ob.vertex_groups.new(name=bone)
            groups[bone].add([v.index], val, 'REPLACE')

# ---- 1つのメッシュに結合してアーマチュアに親子付け ----
bpy.ops.object.select_all(action='DESELECT')
for ob in meshes: ob.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
bpy.ops.object.join()
body = bpy.context.view_layer.objects.active
body.name = body.data.name = "PlayerBody"
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
body.parent = rig
mod = body.modifiers.new("Armature", 'ARMATURE'); mod.object = rig
# ヘルメット等で頂点が重複していてもOK。距離0のマージはしない(パーツの色境界を保つため)

# ---- サンプルアクション: 両手でハンドルを持つ ----
def two_bone_ik(S, W, l1, l2, pole):
    """肩S・手首W(Vector) → 肘の位置。pole 側に肘を出す"""
    d = W - S; dist = min(d.length, l1 + l2 - 1e-4); dn = d.normalized()
    a = (l1*l1 - l2*l2 + dist*dist) / (2 * dist)
    h = math.sqrt(max(l1*l1 - a*a, 0.0))
    pv = pole - S; pv = (pv - dn * pv.dot(dn)).normalized()
    return S + dn * a + pv * h

def aim(pb, head, tail, up=Vector((0, 0, 1))):
    y = (tail - head).normalized()
    z = (up - y * up.dot(y)).normalized(); x = y.cross(z)
    m = Matrix((x, y, z)).transposed().to_4x4(); m.translation = head
    pb.matrix = m
    bpy.context.view_layer.update()

bpy.context.view_layer.objects.active = rig
bpy.ops.object.mode_set(mode='POSE')
pose = rig.pose.bones
act = bpy.data.actions.new("RopeHold"); rig.animation_data_create(); rig.animation_data.action = act

def set_rope_pose(sway):
    for pb in pose: pb.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()
    # 少し前傾 + 左右に揺れる
    pose["spine"].rotation_mode = pose["chest"].rotation_mode = 'XYZ'
    pose["spine"].rotation_euler = (math.radians(12), 0, math.radians(sway * 4))
    pose["chest"].rotation_euler = (math.radians(6), math.radians(sway * 6), 0)
    pose["head"].rotation_mode = 'XYZ'; pose["head"].rotation_euler = (math.radians(-14), math.radians(-sway * 5), 0)
    bpy.context.view_layer.update()
    # ハンドルの位置 (胸の前・腰より少し上) を胸の骨の空間で決める
    chest_m = rig.matrix_world @ pose["chest"].matrix
    chest_rest = rig.matrix_world @ rig.data.bones["chest"].matrix_local
    # 静止姿勢でのハンドル位置 (OBJ座標で 前0.40m・高さ0.26m) を、胸の動きに合わせて動かす
    handle = chest_m @ (chest_rest.inverted() @ B((0.0, 0.26, 0.40)))
    for s, side in ((1, "L"), (-1, "R")):
        S = rig.matrix_world @ pose[f"upper_arm.{side}"].head
        W = handle + Vector((s * 0.09, 0, 0))
        E = two_bone_ik(S, W, 0.33, 0.235, S + Vector((s * 0.30, 0.10, -0.15)))  # 肘は外・後ろ・下へ
        aim(pose[f"upper_arm.{side}"], S, E)
        aim(pose[f"forearm.{side}"], E, W)
        fwd = (W - E).normalized()
        aim(pose[f"hand.{side}"], W, W + fwd * 0.09)

for frame, sway in ((1, -1.0), (31, 1.0), (61, -1.0)):
    set_rope_pose(sway)
    for pb in pose:
        pb.rotation_mode = 'QUATERNION'
        pb.keyframe_insert("rotation_quaternion", frame=frame)
        pb.keyframe_insert("location", frame=frame)
bpy.context.scene.frame_start, bpy.context.scene.frame_end = 1, 61
bpy.context.scene.frame_set(1)
bpy.ops.object.mode_set(mode='OBJECT')

# ---- 保存と glTF 書き出し ----
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "player_upper_rigged.blend"))
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, "player_upper.gltf"), export_format='GLTF_SEPARATE',
                          export_animations=True, export_skins=True, export_yup=True)
print("bones:", len(arm_data.bones), "verts:", len(body.data.vertices), "groups:", [g.name for g in body.vertex_groups])
