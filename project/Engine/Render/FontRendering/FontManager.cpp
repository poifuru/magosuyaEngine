#include "PCH.h"
#include "FontManager.h"

void FontManager::Initialize(TextureManager* textureManager) {
	textureManager_ = textureManager;
}

Font* FontManager::GetFont(const std::string& fontPath, float pixelHeight, bool sdf) {
	// "パス@サイズ(#sdf)" をキーにする（同じファイルでもサイズ違い・焼き方違いは別のアトラス）
	const int sizeKey = static_cast<int>(pixelHeight + 0.5f);
	const std::string key = fontPath + "@" + std::to_string(sizeKey) + (sdf ? "#sdf" : "");

	auto it = fonts_.find(key);
	if (it != fonts_.end()) return it->second.get();

	auto font = std::make_unique<Font>();
	if (!font->Load(fontPath, static_cast<float>(sizeKey), charsetPath_, sdf)) {
		return nullptr;
	}
	font->CreateTexture(textureManager_, "font:" + key);

	Font* ptr = font.get();
	fonts_[key] = std::move(font);
	return ptr;
}
