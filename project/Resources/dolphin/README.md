# Low-poly Dolphin（イルカ）

| ファイル | 内容 |
|---|---|
| dolphin.obj / .mtl | 静的メッシュ（三角形700）。パーツごとに `o`（body, dorsal, pec.L/R, fluke, face） |
| dolphin_palette.png | 64x64 の色パレット（背中の青灰色・お腹の白など） |
| dolphin_rigged.blend | 骨6本＋ウェイト設定済み。泳ぎのループアクション `Swim`（40フレーム）付き |
| dolphin.gltf / .bin | 上の .blend を書き出したスキンメッシュ（Swim アニメーション入り） |
| gen_dolphin.py | メッシュ生成スクリプト（Python + Pillow） |
| blender_rig_dolphin.py | OBJ → 骨・ウェイト・Swim → .blend / .gltf を作る Blender 用スクリプト |
| preview.png | プレビュー（右2枚は Swim の途中のフレーム） |

## 座標系とサイズ
- Y-up / 右手系 / 頭（くちばし）= +Z / 単位メートル
- 全長 約2.6m（くちばし先 z=1.08 〜 尾びれ先 z=-1.52）、胴の太さ 約0.5m、背びれ上端 y=0.58
- 原点 = 胴体の中心（ジャンプで回転させるときの軸）

## 骨
```
body (ルート z=-0.05 → 0.40)
├ head (→ くちばし先)
└ tail1 (z=-0.05 → -0.50) ─ tail2 (→ -0.88) ─ tail3 (→ -1.18) ─ fluke (→ -1.50)
```
- 胴体と背びれは z に沿って隣の骨となめらかにブレンド。胸びれは body、尾びれは fluke、目・口は head に100%
- Swim は骨のローカルX軸まわり（上下）の回転だけ。後ろの骨ほど遅れて大きく振る（3°〜20°）

## エンジンで使うときのメモ
- 既存のローダー前提の標準形式（OBJ / glTF）なので変換は不要
- 泳ぐ速さに合わせて Swim の再生速度を変えると自然に見える（速く泳ぐほど速く振る）
