#include "PCH.h"
#include "ParticleEffectData.h"

using nlohmann::json;
using MyEngine::Rendering::BlendModeType;

namespace {
	// ---- 書き出し用 ----
	json ToArray(const Vector2& v) { return json::array({ v.x, v.y }); }
	json ToArray(const Vector3& v) { return json::array({ v.x, v.y, v.z }); }
	json ToArray(const Vector4& v) { return json::array({ v.x, v.y, v.z, v.w }); }

	// ---- 読み込み用：キーが無い・形が違うときはデフォルト ----
	void ReadVector(const json& j, const char* key, Vector2& out) {
		if (!j.contains(key) || !j[key].is_array() || j[key].size() < 2) { return; }
		out.x = j[key][0].get<float>();
		out.y = j[key][1].get<float>();
	}
	void ReadVector(const json& j, const char* key, Vector3& out) {
		if (!j.contains(key) || !j[key].is_array() || j[key].size() < 3) { return; }
		out.x = j[key][0].get<float>();
		out.y = j[key][1].get<float>();
		out.z = j[key][2].get<float>();
	}
	void ReadVector(const json& j, const char* key, Vector4& out) {
		if (!j.contains(key) || !j[key].is_array() || j[key].size() < 4) { return; }
		out.x = j[key][0].get<float>();
		out.y = j[key][1].get<float>();
		out.z = j[key][2].get<float>();
		out.w = j[key][3].get<float>();
	}
	
	const char* BlendToString(BlendModeType blend) {
		switch (blend) {
		case BlendModeType::Alpha:    return "alpha";
		case BlendModeType::Subtract: return "subtract";
		case BlendModeType::Multiply: return "multiply";
		case BlendModeType::Screen:   return "screen";
		default:                      return "additive";
		}
	}

	BlendModeType BlendFromString(const std::string& s) {
		if (s == "alpha")    { return BlendModeType::Alpha; }
		if (s == "subtract") { return BlendModeType::Subtract; }
		if (s == "multiply") { return BlendModeType::Multiply; }
		if (s == "screen")   { return BlendModeType::Screen; }
		return BlendModeType::Additive;
	}
}

bool LoadParticleEffect(const std::string& path, ParticleEffectData& out) {
	std::ifstream file(path);
	if (!file) { return false; }

	json j = json::parse(file, nullptr, false);  // 第3引数 false = 書き間違いでも例外を投げない
	if (j.is_discarded()) { return false; }      // JSON の文法ミス

	out.version  = j.value("version", 1);
	out.maxCount = j.value("maxCount", out.maxCount);

	if (j.contains("emit")) {
		const json& e = j["emit"];
		out.loop       = e.value("loop", out.loop);
		out.rate       = e.value("rate", out.rate);
		out.burstCount = e.value("burstCount", out.burstCount);
	}
	if (j.contains("look")) {
		const json& l = j["look"];
		ReadVector(l, "startColor", out.startColor);
		ReadVector(l, "endColor", out.endColor);
		ReadVector(l, "startSize", out.startSize);
		out.endSize = l.value("endSize", out.endSize);
		out.texture = l.value("texture", out.texture);
		out.blend   = BlendFromString(l.value("blend", std::string("additive")));
	}
	if (j.contains("shape")) {
		const json& s = j["shape"];
		out.shape       = s.value("type", out.shape);
		out.radius      = s.value("radius", out.radius);
		ReadVector(s, "boxSize", out.boxSize);
		ReadVector(s, "direction", out.direction);
		out.spreadAngle = s.value("angleDeg", out.spreadAngle);
	}
	if (j.contains("life")) {
		const json& lf = j["life"];
		ReadVector(lf, "lifeTime", out.lifeTime);
		ReadVector(lf, "speed", out.speed);
	}
	if (j.contains("force")) {
		const json& f = j["force"];
		ReadVector(f, "acceleration", out.acceleration);
		out.drag = f.value("drag", out.drag);
	}
	return true;
}

bool SaveParticleEffect(const std::string& path, const ParticleEffectData& data) {
	json j;
	j["version"] = data.version;
	j["maxCount"] = data.maxCount;
	j["emit"]  = { {"loop", data.loop}, {"rate", data.rate}, {"burstCount", data.burstCount} };
	j["shape"] = { {"type", data.shape}, {"radius", data.radius}, {"boxSize", ToArray(data.boxSize)},
				  {"direction", ToArray(data.direction)}, {"angleDeg", data.spreadAngle} };
	j["life"]  = { {"lifeTime", ToArray(data.lifeTime)}, {"speed", ToArray(data.speed)} };
	j["force"] = { {"acceleration", ToArray(data.acceleration)}, {"drag", data.drag} };
	j["look"]  = { {"startColor", ToArray(data.startColor)}, {"endColor", ToArray(data.endColor)},
				  {"startSize", ToArray(data.startSize)}, {"endSize", data.endSize},
				  {"texture", data.texture}, {"blend", BlendToString(data.blend)} };

	std::ofstream file(path);
	if (!file) { return false; }
	file << j.dump(2);   // 2 = インデント幅。Git で差分が読みやすくなる
	return true;
}
