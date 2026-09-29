#pragma once

// 前方宣言
struct SceneContext;
struct CameraData;
class GameObject;
namespace MyEngine::LowLevel { class SrvDescriptorHeapPool; }
namespace MyEngine::Rendering { class RenderTexture; }

// ウィンドウで参照したい情報の構造体
struct EditorContext {
	ID3D12Device* device = nullptr;
	MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager = nullptr;
	MyEngine::Rendering::RenderTexture* renderTexture = nullptr;
	SceneContext* sceneContext = nullptr;
	GameObject** selectedObject = nullptr;
	CameraData* cameraData = nullptr;
};

// エディタウィンドウの親クラス
class IEditorWindow {
public:
	IEditorWindow(const std::string& name, bool defaultOpen = true);
	virtual ~IEditorWindow() = default;

	virtual void Initialize();

	// 毎フレーム呼ばれる描画・更新
	virtual void UpdateAndDraw(const EditorContext& context) = 0;

	// ゲッター / セッター
	const std::string& GetName() const { return name_; }
	bool IsOpen() const { return isOpen_; }
	void SetOpen(bool open) { isOpen_ = open; }

	// ImGui::MenuItem や ImGui::Begin にポインタを渡す用
	bool* GetIsOpenPtr() { return &isOpen_; }

protected:
	std::string name_;
	bool isOpen_ = true;
};