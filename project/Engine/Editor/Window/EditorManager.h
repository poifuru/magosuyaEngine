#pragma once
#include "IEditorWindow.h"

namespace MyEngine::LowLevel {
	class SrvDescriptorHeapPool;
}

namespace MyEngine::Rendering {
	class RenderTexture;
}

class GameObject;
struct SceneContext;

class TextureManager;

// エディタ稼働中の状態
enum class EditorPlayState {
	Edit,	// 停止
	Play,	// 再生
	Pause	// 一時停止
};

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

	// 選択中のGameObject管理
	GameObject* GetSelectedObject() const { return selectedObject_; }
	void SetSelectedObject(GameObject* obj) { selectedObject_ = obj; }
	void ClearSelectedObject() { selectedObject_ = nullptr; }

	// シーンコンテキストの受け渡し用
	void SetSceneContext(SceneContext* context) { sceneContext_ = context; }
	SceneContext* GetSceneContext() const { return sceneContext_; }

	// 再生ステートの取得
	EditorPlayState GetPlayState() const { return playState_; }
	bool IsPlaying() const { return playState_ == EditorPlayState::Play; }
	bool IsPaused()  const { return playState_ == EditorPlayState::Pause; }
	bool IsEditing() const { return playState_ == EditorPlayState::Edit; }

	// 再生・一時停止・停止コマンド
	void Play();
	void Pause();
	void Stop();

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

	// 選択中GameObjectのポインタ
	GameObject* selectedObject_ = nullptr;

	// SceneContext
	SceneContext* sceneContext_ = nullptr;

	// 稼働状態
	EditorPlayState playState_ = EditorPlayState::Edit;

	// シーン再生直前の状態を保存するメモリ
	json sceneSnapshot_;
};
