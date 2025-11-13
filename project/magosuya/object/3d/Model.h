#pragma once
#include <vector>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;
#include <memory>
#include "struct.h"
#include "ModelRenderer.h"

class MagosuyaEngine;

class Model {
public:	//メンバ関数
	//コンストラクタ
	/// <summary>
	/// 指定されたディレクトリパスとファイル名から3Dモデルを初期化します。
	/// </summary>
	/// <param name="directoryPath">3Dモデルファイルが存在するディレクトリのパス。</param>
	/// <param name="filename">読み込む3Dモデルのファイル名。</param>
	Model (MagosuyaEngine* magosuya);

	~Model ();

	/// <summary>
	/// モデルのSRT
	/// </summary>
	/// <param name="scale">大きさ</param>
	/// <param name="rotate">回転</param>
	/// <param name="position">位置</param>
	void Initialize (Vector3 scale = { 1.0f, 1.0f, 1.0f }, Vector3 rotate = { 0.0f, 0.0f, 0.0f }, Vector3 position = { 0.0f, 0.0f, 0.0f });
	
	/// <summary>
	/// 更新
	/// </summary>
	/// <param name="vp">vp行列</param>
	void Update (Matrix4x4* vp);

	/// <summary>
	/// 描画処理
	/// </summary>
	/// <param name="cmdList">コマンドリスト</param>
	/// <param name="textureHandle">使うテクスチャ</param>
	/// <param name="light">ライト</param>
	void Draw ();
	
	/// <summary>
	/// ImGuiで編集できるよ
	/// </summary>
	void ImGui ();

	/// <summary>
	/// どのモデルを使うのか
	/// </summary>
	void SetModelData (const std::string& ID);

	void SetTexture (const std::string& ID);

	//アクセッサ
	Transform GetTransform () { return transform_; }
	void SetTransform (Transform transform) { transform_ = transform; }
	Transform GetUVTransform () { return uvTransform_; }
	void SetUVTransform (Transform transform) { uvTransform_ = transform; }

private:		//メンバ変数
	//マネージャーから受け取るモデルデータ
	std::weak_ptr<ModelData> modelData_;
	//貼り付けるテクスチャーのハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE texture_;

	Transform transform_ = {};
	Transform uvTransform_ = {};

	//レンダラークラス
	std::unique_ptr<ModelRenderer> renderer_ = nullptr;

	//ポインタを借りる
	MagosuyaEngine* magosuya_ = nullptr;
};