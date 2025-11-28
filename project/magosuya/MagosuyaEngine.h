#pragma once
#include <memory>
#include <string>

//前方宣言
class WindowsAPI;
class DxCommon;
class ImGuiManager;
class InputManager;
class TextureManager;
class ModelManager;
class PSOManager;
class LineRenderer;
class CubeRenderer;

class MagosuyaEngine {
public:		//メンバ関数
	static MagosuyaEngine* GetInstance () {
		//初めて呼び出されたときに一回だけ初期化
		static MagosuyaEngine instance;
		return &instance;
	}
	~MagosuyaEngine ();

	void Initialize ();
	void BeginFrame ();
	void EndFrame ();

private:
	//コンストラクタを禁止
	MagosuyaEngine () = default;
	// コピーコンストラクタと代入演算子を禁止
	MagosuyaEngine (const MagosuyaEngine&) = delete;
	MagosuyaEngine& operator=(const MagosuyaEngine&) = delete;
	MagosuyaEngine (MagosuyaEngine&&) = delete;
	MagosuyaEngine& operator=(MagosuyaEngine&&) = delete;

private:
	WindowsAPI* winApi_ = nullptr;
	DxCommon* dxCommon_ = nullptr;
	ImGuiManager* imguiManager_ = nullptr;
	InputManager* inputManager_ = nullptr;
	TextureManager* texManager_ = nullptr;
	ModelManager* modelManager_ = nullptr;
	PSOManager* psoManager_ = nullptr;
	LineRenderer* lineRenderer_ = nullptr;
	CubeRenderer* cubeRenderer_ = nullptr;
};