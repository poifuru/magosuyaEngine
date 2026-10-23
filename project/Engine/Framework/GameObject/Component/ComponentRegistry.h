#pragma once
#include "GameObject.h"
#include <vector>

// テンプレートを使って型ごとの関数ポインタを生成する
// 構造体のstatic関数にすることでインスタンスが無くても呼べる
template <typename T>
struct ComponentHelper {
	static void Add(GameObject* obj) {
		if(!obj) return;

		auto* comp = obj->GetComponent<T>();

		if(!comp) {
			comp = obj->AddComponent<T>();
		}
	}

	static void Deserialize(GameObject* obj, const json& j) {
		if(!obj) return;
		auto* comp = obj->GetComponent<T>();
		if(!comp) comp = obj->AddComponent<T>();
		comp->Deserialize(j);
	}

	static bool Has(GameObject* obj) {
		return obj && obj->GetComponent<T>() != nullptr;
	}
};

struct ComponentEntry {
	const char* typeName;		// JSON用
	const char* displayName;	// 表示名
	const char* category;		// カテゴリ

	void (*addFunc)(GameObject*);
	void (*deserializeFunc)(GameObject*, const json&);
	bool (*hasFunc)(GameObject*);
};

class ComponentRegistry {
public:
	// 追加メニュー（ImGui）を描画する
	static void DrawAddComponentMenu(GameObject* obj);

	// JSONからコンポーネントを復元する
	static void DeserializeComponent(GameObject* obj, const json& compJ);

private:
	// 登録テーブルを取得する
	static const std::vector<ComponentEntry>& GetEntries();
};
