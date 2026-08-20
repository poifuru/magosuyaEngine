#pragma once
using json = nlohmann::json;

class GameObject;

namespace MyEngine::Rendering {
	class Renderer;
}

class Component {
public:
	Component() = default;
	virtual ~Component() = default;

	// 基本ライフサイクル
	virtual void Initialize() {}
	virtual void Update() {}
	virtual void ImGui() {}

	// Jsonへの書き出し
	virtual void Serialize(json& j) const {}
	// Jsonからの読み込み
	virtual void Deserialize(const json& j) {}

	// エディタで表示するためのコンポーネント名
	virtual const char* GetName() const = 0;

	// 所属するGameObjectへのアクセス
	void SetGameObject(GameObject* gameObject) { gameObject_ = gameObject; }
	GameObject* GetGameObject() const { return gameObject_; }
	void SetIsDebugMode(bool flag) { isDebugMode_ = flag; }

	// TransformAddressを取得
	D3D12_GPU_VIRTUAL_ADDRESS GetTransformAddress() const;

protected:
	GameObject* gameObject_ = nullptr;	// 親オブジェクトへのポインタ
	bool isDebugMode_ = true;			// デバッグモード中であるか
	bool isInitialized_ = false;		// 初期化ガード用フラグ
};