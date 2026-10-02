#include "PCH.h"
#include "InspectorWindow.h"
#include "GameObject.h"
#include "EditorManager.h"

InspectorWindow::InspectorWindow() 
	: IEditorWindow("インスペクタ", true) {
}

void InspectorWindow::UpdateAndDraw(const EditorContext& context) {
	if (!isOpen_) return;

	ImGui::Begin(name_.c_str(), &isOpen_);

	// EditorManager から今選択されているオブジェクトを取得
	GameObject* selectedObject = EditorManager::GetInstance()->GetSelectedObject();
	if (selectedObject != nullptr) {
		// オブジェクトのパラメータ・コンポーネントを描画
		selectedObject->ImGui();
	}
	else {
		ImGui::Text("オブジェクトが選択されていません");
	}
	ImGui::End();
}