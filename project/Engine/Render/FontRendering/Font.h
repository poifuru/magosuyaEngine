#pragma once

class TextureManager;

// フォントの情報
struct Glyph {
	Vector2 uvMin;
	Vector2 uvMax;
	Vector2 size;           // px
	Vector2 bearing;        // ペン位置からのオフセット px
	float   advance;        // 次の文字まで px
};

// フォント1つ分（1ファイル × 1サイズ）のアトラスと文字情報
class Font {
public:
	/// <summary>
	/// フォントを読み込み、文字をアトラスに焼く
	/// ASCII・ひらがな・カタカナ・記号に加えて、charsetPath のファイルに書いた文字も焼く
	/// </summary>
	/// <param name="fontPath">TTF/OTFのパス(UTF-8)</param>
	/// <param name="pixelHeight">焼くときの文字の高さ px</param>
	/// <param name="charsetPath">追加で焼く文字を書いたUTF-8テキスト（無ければ無視）</param>
	/// <param name="sdf">true なら SDF（輪郭からの距離）で焼く。拡大に強く、縁取り・影が付けられる</param>
	/// <returns>成功したらtrue</returns>
	bool Load(const std::string& fontPath, float pixelHeight, const std::string& charsetPath = "", bool sdf = false);

	/// <summary>
	/// アトラスをPNGに書き出す(確認用)
	/// </summary>
	/// <param name="outPath">出力先のパス(UTF-8)</param>
	/// <returns>成功したらtrue</returns>
	bool SaveAtlasPNG(const std::string& outPath) const;

	/// <summary>
	/// アトラスを GPU テクスチャにする（Load の後に呼ぶ）
	/// </summary>
	/// <param name="textureManager">登録先</param>
	/// <param name="name">登録名（例: "font:HigureGothic"）</param>
	/// <returns>バインドレスヒープ内のインデックス</returns>
	uint32_t CreateTexture(TextureManager* textureManager, const std::string& name);

	// 文字の情報を取得する。無い文字は '?' を返す
	const Glyph* GetGlyph(char32_t c) const;

	uint32_t GetTextureIndex() const { return texIndex_; }
	float GetPixelHeight() const { return pixelHeight_; }
	float GetAscent() const { return ascent_; }
	float GetLineHeight() const { return lineHeight_; }

	bool IsSDF() const { return isSDF_; }
	// SDF の余白 px（縁取り・影のぼかしはこの幅までしか広げられない）
	int GetSdfPadding() const { return kSdfPadding; }

	// UTF-8 の文字列を1文字ずつのコードポイントに分解する
	static std::u32string DecodeUTF8(const std::string& text);

private:
	// 焼く文字の一覧を作る（重複なし・昇順）
	std::vector<int> BuildCodepoints(const std::string& charsetPath) const;

	std::vector<uint8_t> fontData_;     // フォントファイルの中身
	std::vector<uint8_t> atlasPixels_;  // アトラスの画素(1px = 1byte の覆い率)
	int atlasWidth_ = 0;
	int atlasHeight_ = 0;
	std::unordered_map<char32_t, Glyph> glyphs_;

	// SDF の輪郭の外側に取る余白 px
	static constexpr int kSdfPadding = 8;
	bool isSDF_ = false;

	uint32_t texIndex_ = 0;
	float pixelHeight_ = 0.0f;
	float ascent_ = 0.0f;
	float lineHeight_ = 0.0f;
};
