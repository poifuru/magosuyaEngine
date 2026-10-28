#include "PCH.h"
#include "Font.h"
#include "ChangeString.h"
#include "TextureManager.h"
#include <cstring>

// stb_truetype の実装をこのファイルに展開する
// ImGui側(imgui_draw.cpp)も static で実装しているので、こちらも static にして衝突を避ける
// rect_pack を先に入れておくと、stbtt_Pack が文字を隙間なく詰めてくれる
#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#include "imstb_rectpack.h"
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "imstb_truetype.h"

namespace {
	// 試すアトラスの大きさ（小さい順）。入りきらなければ次の大きさで焼き直す
	constexpr int kAtlasSizes[] = { 512, 1024, 2048, 4096 };

	// 指定サイズのアトラスに全部詰められるか試す
	bool TryPack(
		std::vector<uint8_t>& fontData, float pixelHeight, int atlasSize,
		const std::vector<int>& codepoints,
		std::vector<uint8_t>& atlasPixels, std::vector<stbtt_packedchar>& packed
	) {
		atlasPixels.assign(static_cast<size_t>(atlasSize) * atlasSize, 0);
		packed.assign(codepoints.size(), stbtt_packedchar{});

		stbtt_pack_context packContext{};
		// padding = 1: 文字同士の間に1pxの隙間を空けて、バイリニアで隣が滲むのを防ぐ
		if (!stbtt_PackBegin(&packContext, atlasPixels.data(), atlasSize, atlasSize, 0, 1, nullptr)) {
			return false;
		}

		// 飛び飛びの文字を一度に詰めるため、コードポイントの配列で範囲を指定する
		stbtt_pack_range range{};
		range.font_size = pixelHeight;
		range.array_of_unicode_codepoints = const_cast<int*>(codepoints.data());
		range.num_chars = static_cast<int>(codepoints.size());
		range.chardata_for_range = packed.data();

		const int result = stbtt_PackFontRanges(&packContext, fontData.data(), 0, &range, 1);
		stbtt_PackEnd(&packContext);
		return result != 0;
	}

	// SDF の距離が 0〜255 のどこに入るか
	// 文字の輪郭ちょうどが 128、内側ほど大きく、外側ほど小さい
	constexpr unsigned char kSdfOnEdge = 128;

	// SDF で全部の文字を焼いて、アトラスに詰める
	// stbtt_Pack は SDF に対応していないので、焼く(stbtt_GetCodepointSDF)と詰める(stbrp)を自分でやる
	bool BakeSDF(
		const stbtt_fontinfo& info, float scale, const std::vector<int>& codepoints, int padding,
		std::vector<uint8_t>& atlasPixels, int& atlasSize, std::unordered_map<char32_t, Glyph>& glyphs
	) {
		// 距離 1px あたりに値がいくつ変わるか（padding px 離れると 0 になる）
		const float pixelDistScale = static_cast<float>(kSdfOnEdge) / static_cast<float>(padding);

		// 1. 1文字ずつ距離場の画像を作る
		struct BakedGlyph {
			int codepoint = 0;
			unsigned char* bitmap = nullptr;
			int width = 0, height = 0, xoff = 0, yoff = 0;
			float advance = 0.0f;
		};
		std::vector<BakedGlyph> baked;
		baked.reserve(codepoints.size());
		for (int cp : codepoints) {
			// フォントに入っていない文字は飛ばす（'?' で代用される）
			if (cp != ' ' && stbtt_FindGlyphIndex(&info, cp) == 0) continue;

			BakedGlyph g{};
			g.codepoint = cp;
			// 見た目の無い文字(スペースなど)は nullptr が返る
			g.bitmap = stbtt_GetCodepointSDF(&info, scale, cp, padding, kSdfOnEdge, pixelDistScale,
				&g.width, &g.height, &g.xoff, &g.yoff);
			int advanceWidth = 0, leftSideBearing = 0;
			stbtt_GetCodepointHMetrics(&info, cp, &advanceWidth, &leftSideBearing);
			g.advance = advanceWidth * scale;
			baked.push_back(g);
		}

		// 2. 入るサイズのアトラスを探しながら詰める（1px の隙間付き）
		bool packedOk = false;
		std::vector<stbrp_rect> rects(baked.size());
		for (int size : kAtlasSizes) {
			for (size_t i = 0; i < baked.size(); ++i) {
				rects[i] = {};
				rects[i].id = static_cast<int>(i);
				rects[i].w = baked[i].bitmap ? baked[i].width + 1 : 0;
				rects[i].h = baked[i].bitmap ? baked[i].height + 1 : 0;
			}
			std::vector<stbrp_node> nodes(size);
			stbrp_context context{};
			stbrp_init_target(&context, size, size, nodes.data(), static_cast<int>(nodes.size()));
			if (stbrp_pack_rects(&context, rects.data(), static_cast<int>(rects.size()))) {
				atlasSize = size;
				packedOk = true;
				break;
			}
		}

		// 3. 詰めた場所に画素をコピーして、Glyph の表を作る
		if (packedOk) {
			atlasPixels.assign(static_cast<size_t>(atlasSize) * atlasSize, 0);
			const float inv = 1.0f / static_cast<float>(atlasSize);
			for (size_t i = 0; i < baked.size(); ++i) {
				const BakedGlyph& g = baked[i];
				const stbrp_rect& r = rects[i];
				Glyph glyph{};
				glyph.advance = g.advance;
				if (g.bitmap) {
					for (int y = 0; y < g.height; ++y) {
						std::memcpy(&atlasPixels[static_cast<size_t>(r.y + y) * atlasSize + r.x],
							&g.bitmap[static_cast<size_t>(y) * g.width], static_cast<size_t>(g.width));
					}
					glyph.uvMin = { r.x * inv, r.y * inv };
					glyph.uvMax = { (r.x + g.width) * inv, (r.y + g.height) * inv };
					glyph.size = { static_cast<float>(g.width), static_cast<float>(g.height) };
					// SDF の画像は輪郭の外に padding ぶん余白があるので、その分 xoff/yoff もずれている
					glyph.bearing = { static_cast<float>(g.xoff), static_cast<float>(g.yoff) };
				}
				glyphs[static_cast<char32_t>(g.codepoint)] = glyph;
			}
		}

		// 4. stb が確保した画像を解放
		for (BakedGlyph& g : baked) {
			if (g.bitmap) stbtt_FreeSDF(g.bitmap, nullptr);
		}
		return packedOk;
	}
}

bool Font::Load(const std::string& fontPath, float pixelHeight, const std::string& charsetPath, bool sdf) {
	// 1. フォントファイルを丸ごとメモリに読む
	std::ifstream file(std::filesystem::path(String::ConvertString(fontPath)), std::ios::binary | std::ios::ate);
	if (!file) {
		Logger::GetInstance()->Log("Font: フォントが開けません: " + fontPath);
		return false;
	}
	const std::streamsize fileSize = file.tellg();
	file.seekg(0);
	fontData_.resize(static_cast<size_t>(fileSize));
	file.read(reinterpret_cast<char*>(fontData_.data()), fileSize);

	// 2. stb_truetype でフォントを開く
	stbtt_fontinfo info{};
	const int offset = stbtt_GetFontOffsetForIndex(fontData_.data(), 0);
	if (!stbtt_InitFont(&info, fontData_.data(), offset)) {
		Logger::GetInstance()->Log("Font: フォントの解析に失敗: " + fontPath);
		return false;
	}
	pixelHeight_ = pixelHeight;

	// 3. 行の高さなどの情報 (フォント内部の単位 → px)
	const float scale = stbtt_ScaleForPixelHeight(&info, pixelHeight);
	int ascent = 0, descent = 0, lineGap = 0;
	stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
	ascent_ = ascent * scale;
	lineHeight_ = (ascent - descent + lineGap) * scale;

	// 4. 焼く文字を集めて、アトラスに詰める
	const std::vector<int> codepoints = BuildCodepoints(charsetPath);
	glyphs_.clear();
	isSDF_ = sdf;

	if (isSDF_) {
		// SDF: 1文字ずつ距離場を作って、自分で詰める
		int atlasSize = 0;
		if (!BakeSDF(info, scale, codepoints, kSdfPadding, atlasPixels_, atlasSize, glyphs_)) {
			Logger::GetInstance()->Log("Font: 4096x4096 のアトラスにも入りきりません。文字数かサイズを減らしてください");
			return false;
		}
		atlasWidth_ = atlasSize;
		atlasHeight_ = atlasSize;
		return true;
	}

	// ビットマップ: stbtt_Pack に焼くのと詰めるのを両方任せる
	std::vector<stbtt_packedchar> packed;
	bool packedOk = false;
	for (int size : kAtlasSizes) {
		if (TryPack(fontData_, pixelHeight_, size, codepoints, atlasPixels_, packed)) {
			atlasWidth_ = size;
			atlasHeight_ = size;
			packedOk = true;
			break;
		}
	}
	if (!packedOk) {
		Logger::GetInstance()->Log("Font: 4096x4096 のアトラスにも入りきりません。文字数かサイズを減らしてください");
		return false;
	}

	// 5. stb の結果を Glyph の表に変換
	const float invW = 1.0f / static_cast<float>(atlasWidth_);
	const float invH = 1.0f / static_cast<float>(atlasHeight_);
	for (size_t i = 0; i < codepoints.size(); ++i) {
		// フォントに入っていない文字は飛ばす（'?' で代用される）
		if (codepoints[i] != ' ' && stbtt_FindGlyphIndex(&info, codepoints[i]) == 0) continue;

		const stbtt_packedchar& pc = packed[i];
		Glyph glyph{};
		glyph.uvMin = { pc.x0 * invW, pc.y0 * invH };
		glyph.uvMax = { pc.x1 * invW, pc.y1 * invH };
		glyph.size = { static_cast<float>(pc.x1 - pc.x0), static_cast<float>(pc.y1 - pc.y0) };
		// yoff はベースラインから見た文字の上端 (Y下向きなので、上にあるほどマイナス)
		glyph.bearing = { pc.xoff, pc.yoff };
		glyph.advance = pc.xadvance;
		glyphs_[static_cast<char32_t>(codepoints[i])] = glyph;
	}

	return true;
}

std::vector<int> Font::BuildCodepoints(const std::string& charsetPath) const {
	std::vector<int> codepoints;

	// ASCII の表示できる文字
	for (int c = 0x20; c <= 0x7E; ++c) codepoints.push_back(c);
	// 全角の句読点・カッコ (、。「」など)
	for (int c = 0x3000; c <= 0x303F; ++c) codepoints.push_back(c);
	// ひらがな
	for (int c = 0x3041; c <= 0x3096; ++c) codepoints.push_back(c);
	// カタカナ (ー も含む)
	for (int c = 0x30A1; c <= 0x30FC; ++c) codepoints.push_back(c);
	// 全角英数・記号 (！？０〜９など)
	for (int c = 0xFF01; c <= 0xFF5E; ++c) codepoints.push_back(c);

	// charset ファイルに書いた文字（漢字など）
	if (!charsetPath.empty()) {
		std::ifstream file(std::filesystem::path(String::ConvertString(charsetPath)), std::ios::binary);
		std::string line;
		while (std::getline(file, line)) {
			// '#' で始まる行はコメントとして読み飛ばす（先頭のBOMも考慮）
			const std::u32string decoded = DecodeUTF8(line);
			size_t head = (!decoded.empty() && decoded[0] == 0xFEFF) ? 1 : 0;
			if (head < decoded.size() && decoded[head] == U'#') continue;

			for (char32_t c : decoded) {
				// 改行やBOMなどは焼かない
				if (c < 0x20 || c == 0xFEFF) continue;
				codepoints.push_back(static_cast<int>(c));
			}
		}
	}

	// 重複を消して昇順に並べる
	std::sort(codepoints.begin(), codepoints.end());
	codepoints.erase(std::unique(codepoints.begin(), codepoints.end()), codepoints.end());
	return codepoints;
}

bool Font::SaveAtlasPNG(const std::string& outPath) const {
	if (atlasPixels_.empty()) return false;

	// 1ch 8bit のグレースケール画像として書き出す
	DirectX::Image image{};
	image.width = static_cast<size_t>(atlasWidth_);
	image.height = static_cast<size_t>(atlasHeight_);
	image.format = DXGI_FORMAT_R8_UNORM;
	image.rowPitch = static_cast<size_t>(atlasWidth_);
	image.slicePitch = image.rowPitch * image.height;
	image.pixels = const_cast<uint8_t*>(atlasPixels_.data());

	const HRESULT hr = DirectX::SaveToWICFile(
		image, DirectX::WIC_FLAGS_NONE,
		DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
		String::ConvertString(outPath).c_str());
	if (FAILED(hr)) {
		Logger::GetInstance()->Log("Font: PNGの保存に失敗: " + outPath);
		return false;
	}
	return true;
}

uint32_t Font::CreateTexture(TextureManager* textureManager, const std::string& name) {
	if (!textureManager || atlasPixels_.empty()) return 0;

	// 文字の画素は「色」ではなく「覆い率」なので R8_UNORM (sRGBなし) で作る
	texIndex_ = textureManager->CreateTextureFromMemory(
		name,
		static_cast<uint32_t>(atlasWidth_), static_cast<uint32_t>(atlasHeight_),
		DXGI_FORMAT_R8_UNORM,
		atlasPixels_.data(), static_cast<size_t>(atlasWidth_));
	return texIndex_;
}

const Glyph* Font::GetGlyph(char32_t c) const {
	auto it = glyphs_.find(c);
	if (it != glyphs_.end()) return &it->second;

	// 焼いていない文字は '?' で代用して、焼き忘れに気付けるようにする
	it = glyphs_.find(U'?');
	return (it != glyphs_.end()) ? &it->second : nullptr;
}

std::u32string Font::DecodeUTF8(const std::string& text) {
	std::u32string result;
	result.reserve(text.size());

	size_t i = 0;
	while (i < text.size()) {
		const uint8_t c = static_cast<uint8_t>(text[i]);

		// 先頭バイトの上位ビットで、この文字が何バイトかを判定する
		int length = 0;
		char32_t code = 0;
		if (c < 0x80) { length = 1; code = c; }                    // 0xxxxxxx
		else if ((c & 0xE0) == 0xC0) { length = 2; code = c & 0x1F; } // 110xxxxx
		else if ((c & 0xF0) == 0xE0) { length = 3; code = c & 0x0F; } // 1110xxxx
		else if ((c & 0xF8) == 0xF0) { length = 4; code = c & 0x07; } // 11110xxx
		else { ++i; continue; } // 壊れたバイトは飛ばす

		if (i + length > text.size()) break;

		// 2バイト目以降は 10xxxxxx の下位6bitをつなげていく
		bool valid = true;
		for (int k = 1; k < length; ++k) {
			const uint8_t next = static_cast<uint8_t>(text[i + k]);
			if ((next & 0xC0) != 0x80) { valid = false; break; }
			code = (code << 6) | (next & 0x3F);
		}
		if (valid) result.push_back(code);
		i += valid ? length : 1;
	}
	return result;
}
