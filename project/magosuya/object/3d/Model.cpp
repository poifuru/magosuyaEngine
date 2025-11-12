#include "Model.h"
#include <imgui.h>
#include "function.h"
#include "MathFunction.h"
#include "MagosuyaEngine.h"

Model::Model (MagosuyaEngine* magosuya, const std::string& directoryPath, const std::string& filename, bool inversion) {
	magosuya_ = magosuya;
	model_ = LoadObjFile (directoryPath, filename, inversion);
}

Model::~Model () {
	if (vertexBuffer_) {
		vertexBuffer_->Unmap (0, nullptr);
	}
	if (indexBuffer_) {
		indexBuffer_->Unmap (0, nullptr);
	}
	if (matrixBuffer_) {
		matrixBuffer_->Unmap (0, nullptr);
	}
	if (materialBuffer_) {
		materialBuffer_->Unmap (0, nullptr);
	}
}

void Model::Initialize (Vector3 scale, Vector3 rotate, Vector3 position) {
	transform_ = {
		scale,
		rotate,
		position
	};
	uvTransform_ = {
		{ 1.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f }
	};
	vertexData_ = model_.vertices;
	memcpy (vertexDataPtr_, vertexData_.data (), sizeof (VertexData) * model_.vertexCount);
}

void Model::Update (Matrix4x4* view, Matrix4x4* proj) {
	transformationMatrix_.World = MakeAffineMatrix (transform_.scale, transform_.rotate, transform_.translate);
	transformationMatrix_.WVP = Multiply (transformationMatrix_.World, Multiply (*view, *proj));

	matrixData_->World = transformationMatrix_.World;
	matrixData_->WVP = transformationMatrix_.WVP;
	matrixData_->WorldInverseTranspose = Transpose (Inverse (matrixData_->World));

	//uvTranform更新
	materialData_->uvTranform = MakeAffineMatrix (uvTransform_.scale, uvTransform_.rotate, uvTransform_.translate);
}

void Model::Draw (D3D12_GPU_DESCRIPTOR_HANDLE textureHandle) {
	
}

void Model::ImGui () {
	std::string label = "##" + id_;
	if (ImGui::ColorEdit4 (("Color" + label).c_str (), color_)) {
		// 色が変更されたらmaterialDataに反映
		materialData_->color.x = color_[0];
		materialData_->color.y = color_[1];
		materialData_->color.z = color_[2];
		materialData_->color.w = color_[3];
	}
	ImGui::DragFloat3 (("scale" + label).c_str (), &transform_.scale.x, 0.01f);
	ImGui::DragFloat3 (("rotate" + label).c_str (), &transform_.rotate.x, 0.01f);
	ImGui::DragFloat3 (("translate" + label).c_str (), &transform_.translate.x, 0.01f);
	ImGui::DragFloat3 (("UVscale" + label).c_str (), &uvTransform_.scale.x, 0.01f);
	ImGui::DragFloat3 (("UVrotate" + label).c_str (), &uvTransform_.rotate.x, 0.01f);
	ImGui::DragFloat3 (("UVtranslate" + label).c_str (), &uvTransform_.translate.x, 0.01f);
	//ライトの種類を選べるようにする
	static int currentNum = 1;
	const char* lights[] = { "None", "lambert", "halfLambert" };
	if (ImGui::Combo ("ライティング", &currentNum, lights, IM_ARRAYSIZE (lights))) {
		if (currentNum == 0) {
			materialData_->enableLighting = Light::none;
		}
		else if (currentNum == 1) {
			materialData_->enableLighting = Light::lambert;
		}
		else if (currentNum == 2) {
			materialData_->enableLighting = Light::halfLambert;
		}
	}
	ImGui::Separator ();
}