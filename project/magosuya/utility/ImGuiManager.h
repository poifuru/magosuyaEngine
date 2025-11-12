#pragma once

class DxCommon;

class ImGuiManager {
public:
	ImGuiManager (DxCommon* dxCommon);
	~ImGuiManager ();

	void Initialize ();
	void Draw ();
	void BeginFrame ();

private:
	DxCommon* dxCommon_ = nullptr;
};

