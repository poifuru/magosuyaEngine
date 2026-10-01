#include "PCH.h"
#include "AssetBrowserWindow.h"
#include "TextureManager.h"
#include "SrvDescriptorHeapPool.h"
#include "GameObject.h"
#include "ComponentType.h"

AssetBrowserWindow::AssetBrowserWindow()
	: IEditorWindow("アセットブラウザ", true) {
}

void AssetBrowserWindow::LoadIconTexture(TextureManager* texManager) {
	iconFolder_  = texManager->LoadTexture("Resources/Editor/AssetBrowserIcon/icon_folder.png");
	iconModel_   = texManager->LoadTexture("Resources/Editor/AssetBrowserIcon/icon_model.png");
	iconTexture_ = texManager->LoadTexture("Resources/Editor/AssetBrowserIcon/icon_texture.png");
	iconFile_    = texManager->LoadTexture("Resources/Editor/AssetBrowserIcon/icon_file.png");
}

void AssetBrowserWindow::UpdateAndDraw(const EditorContext& context) {
	if (!isOpen_) return;

	ImGui::Begin(name_.c_str(), &isOpen_);

	// 戻るボタンと現在パスの表示
	if (currentDirectory_ != "Resources" && currentDirectory_.has_parent_path()) {
		if (ImGui::Button("Back (↑)")) {
			currentDirectory_ = currentDirectory_.parent_path();
		}
		ImGui::SameLine();
	}

	ImGui::Text("Path: %s", currentDirectory_.generic_string().c_str());
	ImGui::Separator();

	// 2ペイン分割（左：ツリー、右：グリッド）
	ImGui::Columns(2, "AssetBrowserSplit", true);
	static bool setColumnWidth = true;
	if (setColumnWidth) {
		ImGui::SetColumnWidth(0, 180.0f);
		setColumnWidth = false;
	}

	// --- 左ペイン：フォルダツリー ---
	ImGui::BeginChild("FolderTreeChild", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
	if (currentDirectory_ == "Resources") {
		rootFlags |= ImGuiTreeNodeFlags_Selected;
	}

	bool rootOpen = ImGui::TreeNodeEx("Resources", rootFlags);
	if (ImGui::IsItemClicked()) {
		currentDirectory_ = "Resources";
	}

	if (rootOpen) {
		DrawDirectoryTree("Resources");
		ImGui::TreePop();
	}

	ImGui::EndChild();
	ImGui::NextColumn();

	// --- 右ペイン：ファイル一覧グリッド ---
	ImGui::BeginChild("FileGridChild");
	float thumbnailSize = 72.0f; // アイコン画像のサイズ
	float padding = 16.0f;
	float cellSize = thumbnailSize + padding;
	float panelWidth = ImGui::GetContentRegionAvail().x;
	int columnCount = (int)(panelWidth / cellSize);
	if (columnCount < 1) columnCount = 1;

	ImGui::Columns(columnCount, nullptr, false);
	GameObject* selectedObject = (context.selectedObject ? *context.selectedObject : nullptr);

	if (std::filesystem::exists(currentDirectory_) && std::filesystem::is_directory(currentDirectory_)) {
		int id = 0;
		for (const auto& entry : std::filesystem::directory_iterator(currentDirectory_)) {
			ImGui::PushID(id++);
			std::string filename = entry.path().filename().string();
			bool isDir = entry.is_directory();
			std::string ext = entry.path().extension().string();

			// 適切なアイコンのGPUハンドルを取得
			uint32_t iconIndex = GetIconForFile(isDir, ext);
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = context.srvHeap->GetGpuHandle(iconIndex);

			// 画像ボタンとして描画(クリック判定)
			bool isClicked = false;
			if (iconIndex != 0) {
				// アイコン画像がある場合：ImageButton
				isClicked = ImGui::ImageButton(
					filename.c_str(), 
					(ImTextureID)gpuHandle.ptr, 
					ImVec2(thumbnailSize, thumbnailSize)
				);
			} else {
				// アイコン画像がまだ無いときのフォールバック：色付き四角ボタン
				isClicked = ImGui::Button(isDir ? "[Folder]" : ext.c_str(), ImVec2(thumbnailSize, thumbnailSize));
			}

			// ファイル名テキスト
			ImGui::TextWrapped("%s", filename.c_str());

			// クリック時の処理
			if (isClicked) {
				if (isDir) {
					currentDirectory_ = entry.path();
				}
				else if (selectedObject != nullptr) {
					if ((ext == ".obj" || ext == ".gltf")) {
						if (auto* meshRenderer = selectedObject->GetComponent<MeshRendererComponent>()) {
							meshRenderer->SetModel(entry.path().generic_string());
						}
					}
					else if (ext == ".png" || ext == ".jpg" || ext == ".dds") {
						if (auto* meshRenderer = selectedObject->GetComponent<MeshRendererComponent>()) {
							meshRenderer->SetTexture(entry.path().generic_string());
						}

						if (auto* skybox = selectedObject->GetComponent<SkyboxComponent>()) {
							skybox->SetTexture(entry.path().generic_string());
						}

						if (auto* water = selectedObject->GetComponent<WaterSurfaceComponent>()) {
							water->SetTexture(entry.path().generic_string());
						}

						if (auto* sprite = selectedObject->GetComponent<SpriteComponent>()) {
							sprite->SetTexture(entry.path().generic_string());
						}
					}
				}
			}

			// ホバー時のツールチップ（フルファイル名表示）
			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(filename.c_str());
				ImGui::EndTooltip();
			}

			ImGui::NextColumn();
			ImGui::PopID();
		}
	}
	ImGui::Columns(1);
	ImGui::EndChild();
	ImGui::Columns(1);
	ImGui::End();
}

void AssetBrowserWindow::DrawDirectoryTree(const std::filesystem::path& path) {
	if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) return;

	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		if (entry.is_directory()) {
			std::string folderName = entry.path().filename().string();
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

			if (currentDirectory_ == entry.path()) {
				flags |= ImGuiTreeNodeFlags_Selected;
			}

			bool hasSubDir = false;
			for (const auto& subEntry : std::filesystem::directory_iterator(entry.path())) {
				if (subEntry.is_directory()) {
					hasSubDir = true;
					break;
				}
			}

			if (!hasSubDir) {
				flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			}

			bool nodeOpen = ImGui::TreeNodeEx(folderName.c_str(), flags);
			if (ImGui::IsItemClicked()) {
				currentDirectory_ = entry.path();
			}

			if (nodeOpen && hasSubDir) {
				DrawDirectoryTree(entry.path());
				ImGui::TreePop();
			}
		}
	}
}

uint32_t AssetBrowserWindow::GetIconForFile(bool isDirectory, const std::string& extension) const {
	if (isDirectory) return iconFolder_;
	if (extension == ".obj" || extension == ".gltf") return iconModel_;
	if (extension == ".png" || extension == ".jpg" || extension == ".dds") return iconTexture_;
	return iconFile_;
}