#pragma once
#include <vector>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;
#include "engine/engineCore/DxCommon.h"
#include "struct.h"

class Model {
public:	//メンバ関数
	//コンストラクタ
	/// <summary>
	/// 指定されたディレクトリパスとファイル名から3Dモデルを初期化します。
	/// </summary>
	/// <param name="directoryPath">3Dモデルファイルが存在するディレクトリのパス。</param>
	/// <param name="filename">読み込む3Dモデルのファイル名。</param>
	Model (DxCommon* dxCommon, const std::string& directoryPath, const std::string& filename, bool inversion = false);

	~Model ();

	/// <summary>
	/// モデルのSRT
	/// </summary>
	/// <param name="scale">大きさ</param>
	/// <param name="rotate">回転</param>
	/// <param name="position">位置</param>
	void Initialize (Vector3 scale = { 1.0f, 1.0f, 1.0f }, Vector3 rotate = { 0.0f, 0.0f, 0.0f }, Vector3 position = { 0.0f, 0.0f, 0.0f });
	
	/// <summary>
	/// 更新処理
	/// </summary>
	/// <param name="view">ビュー行列</param>
	/// <param name="proj">射影行列</param>
	void Update (Matrix4x4* view, Matrix4x4* proj);

	/// <summary>
	/// 描画処理
	/// </summary>
	/// <param name="cmdList">コマンドリスト</param>
	/// <param name="textureHandle">使うテクスチャ</param>
	/// <param name="light">ライト</param>
	void Draw (D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);
	
	/// <summary>
	/// ImGuiで編集できるよ
	/// </summary>
	void ImGui ();

	//アクセッサ
	ModelData GetModelData () { return model_; }
	void SetPositon (Vector3 pos) { transform_.translate = pos; }

private:		//メンバ変数
	//モデルデータ
	ModelData model_;

	//頂点データ
	ComPtr<ID3D12Resource> vertexBuffer_;
	std::vector<VertexData> vertexData_;
	VertexData* vertexDataPtr_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vbView_{};

	//インデックスデータ
	ComPtr<ID3D12Resource> indexBuffer_;
	D3D12_INDEX_BUFFER_VIEW ibView_{};

	//行列データ
	ComPtr<ID3D12Resource> matrixBuffer_;
	TransformationMatrix* matrixData_ = nullptr;
	Transform transform_;	//ローカル座標
	Transform uvTransform_;	//uvTranform用のローカル座標
	TransformationMatrix transformationMatrix_; //ワールド座標と、カメラからwvp行列をもらって格納する

	//マテリアルデータ
	ComPtr<ID3D12Resource> materialBuffer_;
	Material* materialData_ = nullptr;;

	//ImGui用のラベル名
	std::string id_;

	//ImGuiで色をいじる変数
	float color_[4];

	//ポインタを持たせておく
	DxCommon* dxCommon_ = nullptr;
};