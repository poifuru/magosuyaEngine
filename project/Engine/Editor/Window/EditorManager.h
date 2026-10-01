#pragma once
#include "IEditorWindow.h"

namespace MyEngine::LowLevel {
	class SrvDescriptorHeapPool;
}

namespace MyEngine::Rendering {
	class RenderTexture;
}

class GameObject;
class SceneContext;
class TextureManager;

class EditorManager {
public:
	static EditorManager* GetInstance() {
		static EditorManager instance;
		return &instance;
	}
	~EditorManager() = default;

	// 初期化と終了処理（ウィンドウ登録や設定のロード/セーブ）
	void Initialize();
	void Finalize();

	// 毎フレームImGuiManagerで呼び出す
	void UpdateAndDraw(
		ID3D12Device* device,
		MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager,
		MyEngine::Rendering::RenderTexture* renderTexture
	);

	// アセットブラウザウィンドウのアイコン取得用
	void LoadAssetBrowserIcon(TextureManager* texManager);

	// 任意のウィンドウ型を登録する
	template <typename T, typename... Args>
	T* RegisterWindow(Args&&... args) {
		auto window = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr = window.get();
		windows_.push_back(std::move(window));
		return ptr;
	}

	// 任意のウィンドウ型を取得する
	template <typename T>
	T* GetWindow() const {
		for (auto& window : windows_) {
			T* ptr = dynamic_cast<T*>(window.get());
			if (ptr) {
				return ptr;
			}
		}
		return nullptr;
	}

	// 外部がゲーム画面の状態を知るためのゲッター
	bool IsGameWindowHovered() const;
	bool IsGameWindowFocused() const;
	ImVec2 GetGameScreenPos() const;
	ImVec2 GetGameScreenSize() const;

	// ギズモがアクティブかどうかを設定・取得する
	void SetGizmoActive(bool active);
	bool IsGizmoActive() const;

	// レイアウト設定の保存と復元
	void SaveLayoutSettings();
	void LoadLayoutSettings();

private:
	EditorManager() = default;
	EditorManager(const EditorManager&) = delete;
	EditorManager& operator=(const EditorManager&) = delete;

	// メニューバー表示
	void DrawMenuBar();

private:
	// エディタウィンドウクラス配列
	std::vector<std::unique_ptr<IEditorWindow>> windows_;

	// エディタのレイアウト保存用
	const std::string settingsFilePath_ = "Resources/Editor/editor_settings.json";

	// 選択中のGameObject管理

};
