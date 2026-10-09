#pragma once
#include "Component.h"
#include "ConstantBuffer.h"
#include "TransformMatrixData.h"
#include "MeshData.h"

// 前方宣言
namespace MyEngine::Rendering {
	class Material;
	enum class InputLayoutType : uint32_t;
}

struct SkyboxVertex {
	Vector4 position;
};

class SkyboxComponent : public Component {
public:
	SkyboxComponent() = default;
	~SkyboxComponent() override;

	void Initialize() override;
	void Update() override;

	const MyEngine::Rendering::Mesh& GetMesh();
	MyEngine::Rendering::Material* GetMaterial();
	D3D12_GPU_VIRTUAL_ADDRESS GetTransformAddress();
	
	void SetTexture(const std::string& filePath);
	const std::string& GetTexturePath() const { return texturePath_; }

	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;
	MyEngine::Rendering::InputLayoutType GetInputLayoutType();

	const char* GetName() const override { return "SkyboxComponent"; }

private:
	// メッシュ構造体
	MyEngine::Rendering::Mesh mesh_;

	// バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vbv_{};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_ = nullptr;
	D3D12_INDEX_BUFFER_VIEW ibv_{};
	std::unique_ptr<ConstantBuffer<TransformMatrixData>> matrixBuffer_;

	// マッピング用のポインタ
	SkyboxVertex* vertexData_ = nullptr;
	uint32_t* indexData_ = nullptr;

	// テクスチャインデックス
	uint32_t textureIndex_ = 0;
	std::string texturePath_;

	std::shared_ptr<MyEngine::Rendering::Material> material_ = nullptr;
};