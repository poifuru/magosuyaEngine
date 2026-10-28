# Blender 用: dolphin.obj に骨とウェイトを付け、泳ぎのアクション "Swim" を入れて
# dolphin_rigged.blend と dolphin.gltf を書き出すスクリプト
#   blender -b -P blender_rig_dolphin.py -- <dolphin.obj> <出力フォルダ>
#   (または Scripting タブで実行。OBJ と同じフォルダに置く)
# 骨: body(ルート) ─ head / tail1 ─ tail2 ─ tail3 ─ fluke  (計6本)
import bpy, math, os, sys
from mathutils import Vector, Euler

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OBJ = argv[0] if argv else os.path.join(os.path.dirname(bpy.data.filepath or __file__), "dolphin.obj")
OUT = argv[1] if len(argv) > 1 else os.path.dirname(os.path.abspath(OBJ))

def B(p): return Vector((p[0], -p[2], p[1]))     # OBJ (Y-up, 頭+Z) → Blender (Z-up)
def O(v): return (v.x, v.z, -v.y)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.wm.obj_import(filepath=OBJ, forward_axis='NEGATIVE_Z', up_axis='Y')
meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']

BONES = {  # name: (head, tail, parent)  OBJ座標
    "body":  ((0, 0.03, -0.05), (0, 0.03, 0.40), None),
    "head":  ((0, 0.03, 0.40), (0, -0.02, 1.08), "body"),
    "tail1": ((0, 0.03, -0.05), (0, 0.035, -0.50), "body"),
    "tail2": ((0, 0.035, -0.50), (0, 0.035, -0.88), "tail1"),
    "tail3": ((0, 0.035, -0.88), (0, 0.03, -1.18), "tail2"),
    "fluke": ((0, 0.03, -1.18), (0, 0.03, -1.50), "tail3"),
}
arm_data = bpy.data.armatures.new("DolphinRig")
rig = bpy.data.objects.new("DolphinRig", arm_data)
bpy.context.scene.collection.objects.link(rig)
bpy.context.view_layer.objects.active = rig
bpy.ops.object.mode_set(mode='EDIT')
for name, (h, t, parent) in BONES.items():
    eb = arm_data.edit_bones.new(name); eb.head, eb.tail = B(h), B(t); eb.roll = 0.0
for name, (h, t, parent) in BONES.items():
    if parent:
        eb = arm_data.edit_bones[name]; eb.parent = arm_data.edit_bones[parent]
        eb.use_connect = name not in ("tail1",)
bpy.ops.object.mode_set(mode='OBJECT')

# ---- ウェイト: z に沿って隣の骨と滑らかにブレンド ----
# (境目のz, 半幅, 前側の骨, 後ろ側の骨)  前から順に
JOINTS = [(0.40, 0.12, "head", "body"), (-0.05, 0.15, "body", "tail1"),
          (-0.50, 0.12, "tail1", "tail2"), (-0.88, 0.10, "tail2", "tail3"), (-1.18, 0.05, "tail3", "fluke")]
def chain_weights(z):
    for zj, hw, front, back in JOINTS:
        if z > zj + hw: return {front: 1.0}
        if z >= zj - hw:
            t = (zj + hw - z) / (2 * hw)
            t = t * t * (3 - 2 * t)
            return {front: 1 - t, back: t}
    return {"fluke": 1.0}
def weights_for(part, p):
    if part in ("body", "dorsal"): return chain_weights(p[2])
    if part.startswith("pec"): return {"body": 1.0}
    if part == "fluke": return {"fluke": 1.0}
    if part == "face": return {"head": 1.0}
    raise RuntimeError("unknown part " + part)

for ob in meshes:
    part = ob.name.split(".0")[0]
    groups = {}
    for v in ob.data.vertices:
        for bone, val in weights_for(part, O(ob.matrix_world @ v.co)).items():
            if val <= 1e-4: continue
            if bone not in groups: groups[bone] = ob.vertex_groups.new(name=bone)
            groups[bone].add([v.index], val, 'REPLACE')

bpy.ops.object.select_all(action='DESELECT')
for ob in meshes: ob.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
bpy.ops.object.join()
body = bpy.context.view_layer.objects.active
body.name = body.data.name = "DolphinBody"
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
body.parent = rig
mod = body.modifiers.new("Armature", 'ARMATURE'); mod.object = rig

# ---- 泳ぎ: 尾を上下に振る波 (後ろの骨ほど遅れて大きく振る) 40フレームでループ ----
bpy.context.view_layer.objects.active = rig
bpy.ops.object.mode_set(mode='POSE')
pose = rig.pose.bones
rig.animation_data_create(); rig.animation_data.action = bpy.data.actions.new("Swim")
AMP = {"body": (3, 0.0), "head": (4, math.pi), "tail1": (8, 0.6), "tail2": (12, 1.2), "tail3": (16, 1.8), "fluke": (20, 2.4)}
LOOP = 40
for f in range(0, LOOP + 1, 5):
    ph = 2 * math.pi * f / LOOP
    for name, (deg, lag) in AMP.items():
        pb = pose[name]; pb.rotation_mode = 'QUATERNION'
        pb.rotation_quaternion = Euler((math.radians(deg) * math.sin(ph - lag), 0, 0)).to_quaternion()
        pb.keyframe_insert("rotation_quaternion", frame=f + 1)
bpy.context.scene.frame_start, bpy.context.scene.frame_end = 1, LOOP
bpy.context.scene.frame_set(1)
bpy.ops.object.mode_set(mode='OBJECT')

os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "dolphin_rigged.blend"))
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, "dolphin.gltf"), export_format='GLTF_SEPARATE',
                          export_animations=True, export_skins=True, export_yup=True)
print("bones:", len(arm_data.bones), "verts:", len(body.data.vertices))
