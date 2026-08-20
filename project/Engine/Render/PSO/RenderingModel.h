#pragma once

namespace MyEngine::Rendering {
	// マテリアルの状態定義
	enum class ShadingModel : uint8_t {
		Standard,		// 通常のPBRレンダリング
		UnLighting,		// ライティング無し
		Skybox,			// スカイボックス
		Particle,		// パーティクル
		WaterSurface,	// 水面
		BoatWake,		// 航跡波
		OutlineObject,	// 単体オブジェクト用アウトライン

		// ポストエフェクト用
		PostEffect_CopyImage,		// コピー
		PostEffect_Outline,			// アウトライン
		PostEffect_ColorGrading,	// カラーグレーティング
		PostEffect_Fog,				// フォグ
		PostEffect_Vignette,		// ヴィネット
		PostEffect_RadialBlur,		// ラジアルブラー
		PostEffect_Dissolve,		// ディゾルブ
		PostEffect_RandomNoise,		// ランダムノイズ

		Count			// 設定の個数
	};

	// ブレンドモードの定義
	enum class BlendModeType : uint8_t {
		Opaque,     // 不透明
		Alpha,      // アルファブレンド
		Additive,   // 加算合成
		Subtract,   // 減算合成
		Multiply,   // 乗算合成
		Screen,     // スクリーン合成

		Count		// 設定の個数
	};

	// インプットレイアウトの定義
	enum class InputLayoutType : uint32_t {
		Standard3D,			//POSITION, TEXCOORD, NORMALを持つレイアウト
		SkinningStandard3D,	// Standard3Dの設定にSkinning用の設定を足したもの
		Particle,			// Particle用
		LineMesh,			// LineMesh描画用
		CubeMesh,			// CubeMesh描画用
		MeshShader,			// 将来のメッシュシェーダー用(InputLayout不要)
		Skybox,				// Skybox描画用
		PostProcess,		// PostEffect用(InputLayout不要)
		BoatWake,			// 航跡波用

		Count				// 設定の個数
	};
}