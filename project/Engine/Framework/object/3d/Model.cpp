#include "PCH.h"
#include "Model.h"
#include "MeshData.h"
#include "BaseCamera.h"

namespace {
	void TraverseAndUpdateNodeMeshBuffers(
		const MyEngine::Rendering::Node* node,
		const Matrix4x4& parentMatrix,
		const Matrix4x4& modelWorld,
		const CameraData* cameraData,
		std::vector<std::unique_ptr<TransformMatrixResource>>& meshBuffers,
		std::vector<bool>& updatedFlags
	) {
		if (!node) return;

		Matrix4x4 localMat = Math::MakeAffineMatrix(
			node->transform.scale, node->transform.rotate, node->transform.translate
		);
		Matrix4x4 currentLocal = parentMatrix * localMat;
		Matrix4x4 meshWorld = currentLocal * modelWorld;

		for (uint32_t meshIdx : node->meshIndices) {
			if (meshIdx < meshBuffers.size() && meshBuffers[meshIdx]) {
				TransformMatrixData data;
				data.World = meshWorld;
				data.WVP = meshWorld * cameraData->vp;
				data.WorldInverseTranspose = Math::Inverse(Math::Transpose(meshWorld));

				meshBuffers[meshIdx]->Update(data);
				updatedFlags[meshIdx] = true;
			}
		}

		for (const auto& child : node->children) {
			TraverseAndUpdateNodeMeshBuffers(&child, currentLocal, modelWorld, cameraData, meshBuffers, updatedFlags);
		}
	}
}

MyEngine::Rendering::Model::Model() : BaseObject3d() {
}

void MyEngine::Rendering::Model::Initialize(MyEngine::Rendering::ModelData* modelData, ID3D12Device* device) {
	BaseObject3d::Initialize();
	modelData_ = modelData;

	if (modelData_ && device) {
		meshTransformBuffers_.resize(modelData_->meshes.size());
		for (size_t i = 0; i < modelData_->meshes.size(); ++i) {
			meshTransformBuffers_[i] = std::make_unique<TransformMatrixResource>();
			meshTransformBuffers_[i]->Initialize(device);
		}
	}
}

void MyEngine::Rendering::Model::Update(const CameraData* cameraData) {
	BaseObject3d::Update(cameraData);

	if (!modelData_ || meshTransformBuffers_.empty()) return;

	Matrix4x4 modelWorld = CalculateWorldMatrix();
	std::vector<bool> updatedFlags(meshTransformBuffers_.size(), false);

	TraverseAndUpdateNodeMeshBuffers(
		&modelData_->rootNode,
		Math::MakeIdentity4x4(),
		modelWorld,
		cameraData,
		meshTransformBuffers_,
		updatedFlags
	);

	for (size_t i = 0; i < meshTransformBuffers_.size(); ++i) {
		if (!updatedFlags[i] && meshTransformBuffers_[i]) {
			TransformMatrixData data;
			data.World = modelWorld;
			data.WVP = modelWorld * cameraData->vp;
			data.WorldInverseTranspose = Math::Inverse(Math::Transpose(modelWorld));
			meshTransformBuffers_[i]->Update(data);
		}
	}
}

D3D12_GPU_VIRTUAL_ADDRESS MyEngine::Rendering::Model::GetMeshTransformGPUAddress(uint32_t meshIndex) const {
	if (meshIndex < meshTransformBuffers_.size() && meshTransformBuffers_[meshIndex]) {
		return meshTransformBuffers_[meshIndex]->GetGPUVirtualAddress();
	}
	return GetTransformGPUAddress();
}

MyEngine::Rendering::Material * MyEngine::Rendering::Model::GetMaterial()
{
	return material_.get();
}

void MyEngine::Rendering::Model::SetShaders(MyEngine::Rendering::ShadingModel shadingModel)  {
	if (material_) {
		material_->SetShadingModel(shadingModel);
	}
}

void MyEngine::Rendering::Model::SetTextureIndex(uint32_t textureIndex)  {
	if (material_) {
		material_->SetTextureIndex(textureIndex);
	}
}

MyEngine::Rendering::Node* MyEngine::Rendering::Model::FindNode(const std::string& name) {
	if (!modelData_) return nullptr;
	return MyEngine::Rendering::FindNode(&modelData_->rootNode, name);
}

const MyEngine::Rendering::Node* MyEngine::Rendering::Model::FindNode(const std::string& name) const {
	if (!modelData_) return nullptr;
	return MyEngine::Rendering::FindNode(&modelData_->rootNode, name);
}

void MyEngine::Rendering::Model::UpdateNodeTransforms() {
	if (!modelData_) return;
	MyEngine::Rendering::UpdateNodeTransforms(&modelData_->rootNode);
}