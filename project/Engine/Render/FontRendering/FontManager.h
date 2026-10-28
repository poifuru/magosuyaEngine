#pragma once
#include "Font.h"

class TextureManager;

// フォントをまとめて持つクラス
// 同じファイル・同じサイズのフォントは1回だけ焼いて、コンポーネント同士で共有する
class FontManager {
public:
	void Initialize(TextureManager* textureManager);

	/// <summary>
	/// フォントを取得する。初めて使う組み合わせならその場で焼いて GPU に送る
	/// </summary>
	/// <param name="fontPath">TTF/OTFのパス(UTF-8)</param>
	/// <param name="pixelHeight">焼くときの文字の高さ px</param>
	/// <returns>失敗したら nullptr</returns>
	Font* GetFont(const std::string& fontPath, float pixelHeight);

	// 追加で焼く文字（漢字など）を書いたファイル
	void SetCharsetPath(const std::string& path) { charsetPath_ = path; }

private:
	TextureManager* textureManager_ = nullptr;
	std::unordered_map<std::string, std::unique_ptr<Font>> fonts_;
	std::string charsetPath_ = "Resources/Font/charset.txt";
};
