#include "GizmoWindow.h"
#include "PCH.h"
#include "GizmoWindow.h"

GizmoWindow::GizmoWindow() 
	: IEditorWindow("ギズモ", true) {
}

void GizmoWindow::UpdateAndDraw(const EditorContext& context) {
	if (!isOpen_) return;

	if (ImGui::Begin(name_.c_str(), &isOpen_)) {
		ImGui::Text("Gizmo Operation");

		if (ImGui::RadioButton("S (Scale)", currentOperation_ == ImGuizmo::SCALE)) {
			currentOperation_ = ImGuizmo::SCALE;
		}
		ImGui::SameLine();

		if (ImGui::RadioButton("R (Rotate)", currentOperation_ == ImGuizmo::ROTATE)) {
			currentOperation_ = ImGuizmo::ROTATE;
		}
		ImGui::SameLine();

		if (ImGui::RadioButton("T (Translate)", currentOperation_ == ImGuizmo::TRANSLATE)) {
			currentOperation_ = ImGuizmo::TRANSLATE;
		}
		ImGui::Separator();

		ImGui::Text("Gizmo Space");
		if (ImGui::RadioButton("Local", currentMode_ == ImGuizmo::LOCAL)) {
			currentMode_ = ImGuizmo::LOCAL;
		}
		ImGui::SameLine();

		if (ImGui::RadioButton("World", currentMode_ == ImGuizmo::WORLD)) {
			currentMode_ = ImGuizmo::WORLD;
		}
	}
	ImGui::End();
}