#include "PCH.h"
#include "ComponentRegistry.h"
#include "ComponentType.h"

// 登録を1行で書ける便利マクロ（型名から自動で文字列化 & 関数ポインタ取得）
#define REGISTER_COMPONENT(Type, DisplayName, Category) \
	{ #Type, DisplayName, Category, \
	  &ComponentHelper<Type>::Add, \
	  &ComponentHelper<Type>::Deserialize, \
	  &ComponentHelper<Type>::Has }

#define REGISTER_COMPONENT_NAMED(Type, TypeName, DisplayName, Category) \
	{ TypeName, DisplayName, Category, \
	  &ComponentHelper<Type>::Add, \
	  &ComponentHelper<Type>::Deserialize, \
	  &ComponentHelper<Type>::Has }

// 登録テーブル本体
const std::vector<ComponentEntry>& ComponentRegistry::GetEntries() {
	static const std::vector<ComponentEntry> entries = {
		// --- Rendering ---
		REGISTER_COMPONENT_NAMED(MyEngine::Rendering::MeshRendererComponent, "MeshRendererComponent", "Mesh Renderer", "Rendering"),
		REGISTER_COMPONENT(SpriteComponent, "Sprite", "Rendering"),
		REGISTER_COMPONENT(SkyboxComponent, "Skybox", "Rendering"),
		REGISTER_COMPONENT(WaterSurfaceComponent, "Water Surface", "Rendering"),
		REGISTER_COMPONENT(BoatWakeComponent, "Boat Wake", "Rendering"),
		REGISTER_COMPONENT(CourseBuoyComponent, "Course Buoys", "Rendering"),
		REGISTER_COMPONENT(NumberDrawerComponent, "Number Drawer", "Rendering"),
		REGISTER_COMPONENT(TextDrawerComponent, "Text Drawer", "Rendering"),
		REGISTER_COMPONENT(FontTextComponent, "Font Text", "Rendering"),
		// --- Camera ---
		REGISTER_COMPONENT(MainCameraComponent, "Main Camera", "Camera"),
		REGISTER_COMPONENT(VirtualFollowCamera, "Virtual Follow Camera", "Camera"),
		REGISTER_COMPONENT(VirtualDebugCamera, "Virtual Debug Camera", "Camera"),
		// --- Light ---
		REGISTER_COMPONENT(LightComponent, "Light", "Lighting"),
		// --- Gameplay ---
		REGISTER_COMPONENT(ColliderComponent, "Collider", "Gameplay"),
		REGISTER_COMPONENT(FloatingCrateComponent, "Floating Crate", "Gameplay"),
		REGISTER_COMPONENT(BoarderComponent, "Boarder", "Gameplay"),
		REGISTER_COMPONENT(BoatDriverComponent, "Boat Driver", "Gameplay"),
		REGISTER_COMPONENT(JumpHeightHudComponent, "Jump Height HUD", "Gameplay"),
		REGISTER_COMPONENT(GameFlowComponent, "Game Flow", "Gameplay"),
		REGISTER_COMPONENT(JumpCountHudComponent, "Jump Count HUD", "Gameplay"),
		REGISTER_COMPONENT(GameMessageHudComponent, "Game Message HUD", "Gameplay"),
		REGISTER_COMPONENT(DolphinEventComponent, "Dolphin Event", "Gameplay"),
		REGISTER_COMPONENT(EnergyItemComponent, "Energy Item", "Gameplay"),
	};
	return entries;
}

// 追加メニューの描画
void ComponentRegistry::DrawAddComponentMenu(GameObject* obj) {
	if (!obj) return;
	const auto& entries = GetEntries();
	// 表示したいカテゴリ一覧
	const char* categories[] = { "Rendering", "Camera", "Lighting", "Gameplay" };
	for (size_t c = 0; c < sizeof(categories) / sizeof(categories[0]); ++c) {
		const char* category = categories[c];
		// カテゴリごとのサブメニューを開く
		if (ImGui::BeginMenu(category)) {
			for (size_t i = 0; i < entries.size(); ++i) {
				const auto& entry = entries[i];
				if (strcmp(entry.category, category) != 0) continue;
				// すでに追加されているかチェック
				if (!entry.hasFunc(obj)) {
					if (ImGui::MenuItem(entry.displayName)) {
						entry.addFunc(obj); // 追加実行！
					}
				} else {
					ImGui::TextDisabled("%s (Already Added)", entry.displayName);
				}
			}
			ImGui::EndMenu();
		}
	}
}

// JSONからの復元
void ComponentRegistry::DeserializeComponent(GameObject* obj, const json& compJ) {
	if (!obj || !compJ.is_object() || !compJ.contains("type")) return;
	std::string type = compJ["type"];
	const auto& entries = GetEntries();
	// テーブルから該当する型名を検索して復元関数を実行（ラムダ式なしループ）
	for (size_t i = 0; i < entries.size(); ++i) {
		if (type == entries[i].typeName) {
			entries[i].deserializeFunc(obj, compJ);
			return;
		}
	}
}
