#pragma once
#include "IEditorWindow.h"

class TextureManager;

class AssetBrowserWindow : public IEditorWindow {
public:
	// コンストラクタ
	AssetBrowserWindow();

	// デストラクタ
	~AssetBrowserWindow() override = default;

	// アイコン用画像をロードする
	void LoadIconTexture(TextureManager* texManager);

	// 更新と描画
	void UpdateAndDraw(const EditorContext& context) override;

private:
	// ディレクトリツリーの描画
	void DrawDirectoryTree(const std::filesystem::path& path);

	// 拡張子から適切なアイコンのSRVインデックスを返す
	uint32_t GetIconForFile(bool isDirectory, const std::string& extension) const;

private:
	std::filesystem::path currentDirectory_ = "Resources";

	// 各アイコン画像のSRVインデックス
	uint32_t iconFolder_ = 0;
	uint32_t iconModel_ = 0;
	uint32_t iconTexture_ = 0;
	uint32_t iconFile_ = 0;
};