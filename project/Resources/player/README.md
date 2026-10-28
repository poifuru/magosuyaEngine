# Low-poly Upper-body Player（上半身プレイヤー / ウェイクボーダー, Mii風）

頭は大きめ（通常の1.6倍）、手は指なしの丸い玉、脚とブーツはなしでボードの上に浮いている。

| ファイル | 内容 |
|---|---|
| player_upper.obj / .mtl | 静的メッシュ（三角形1848）。パーツごとに `o` を分けてある（torso, neck, head, helmet, arm.L/R, hand.L/R, board） |
| player_palette.png | 64x64 の色パレット（16pxマス） |
| player_upper_rigged.blend | 骨11本＋ウェイト設定済み。サンプルアクション `RopeHold` 付き（Blender 5.2 で作成） |
| player_upper.gltf / .bin | 上の .blend をそのまま書き出したスキンメッシュ（RopeHold アニメーション入り） |
| gen_player.py | メッシュ生成スクリプト（Python + Pillow） |
| blender_rig.py | OBJ → 骨・ウェイト・サンプルアクション → .blend と .gltf を作る Blender 用スクリプト |
| preview.png | Tポーズと RopeHold ポーズのプレビュー |

## 座標系とサイズ（OBJ / glTF 共通、Y-up）
- 正面 +Z、単位メートル、**原点 = ボード底面の中心**（position.y を水面の高さにすれば板が水面に乗る）
- 腰は y=0.85（下側は丸く閉じてある）。腰とボードの間（約0.7m）は脚なしで空いている
- ボード: 長さ1.40m（長手方向 = X。横向きに乗る）、幅0.42m、両端が0.07m反り上がるロッカー付き。裏にフィン4枚
- Tポーズ。左腕 = +X、右腕 = -X（Blender の .L / .R と同じ）
- ボード底面〜頭頂（ヘルメット込み）約1.80m、両手の先から先まで約1.8m

## 骨（親子関係）
下の数値は腰 = y0 の座標（OBJ上は +0.85 した位置）。
```
board (ルート。ボード底面の中心 → 0.25上)  ※ボードはこの骨に100%
└ spine (腰 0.00 → 0.22)
   └ chest (0.22 → 0.44)
   ├ neck (0.44 → 0.57)
   │  └ head (0.57 → 0.97)
   ├ upper_arm.L (肩 x=0.16 → 肘 x=0.49, y=0.415) ─ forearm.L (→ 手首 0.725) ─ hand.L (→ 0.815)
   └ upper_arm.R （左右対称）
```
- ウェイトは関節まわりだけ2本の骨にブレンド（腰 y=0.18〜0.26、肘 x=0.45〜0.53、手首 x=0.69〜0.73）。それ以外は1本の骨に100%
- 1頂点あたり最大2本

## Blender でアニメーションを付ける流れ
1. player_upper_rigged.blend を開く（PlayerRig を選んでポーズモード）
2. 新しいアクションを作ってキーを打つ。RopeHold は参考用なので消しても上書きしてもよい
- 全身の移動・回転（ジャンプ・着水）は board、しゃがみ・溜め・上体のひねりは spine を動かす
3. File > Export > glTF 2.0 で書き出す（Format: glTF Separate、Animation にチェック）
- 形を変えたいときは gen_player.py を直して再実行し、`blender -b -P blender_rig.py -- player_upper.obj <出力先>` で骨入れまでやり直せる（作ったアクションは .blend からコピーしておくこと）
