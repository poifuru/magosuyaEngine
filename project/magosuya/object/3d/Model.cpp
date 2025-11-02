#include "Model.h"
#include "../../general/function.h"
#include "../../general/Math.h"

Model::Model (ID3D12Device* device, const std::string& directoryPath, const std::string& filename, bool inversion) {
	model_ = LoadObjFile (directoryPath, filename, inversion);

	//===リソースの初期化===//
	//頂点データ
	vertexData_.resize (model_.vertexCount);
	vertexBuffer_ = CreateBufferResource (device, sizeof (VertexData) * model_.vertexCount);
	vertexBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&vertexDataPtr_));
	vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress ();
	vbView_.SizeInBytes = UINT (sizeof (VertexData) * vertexData_.size ());
	vbView_.StrideInBytes = sizeof (VertexData);

	//お前は今じゃない、また今度な！
	//indexBuffer_ = CreateBufferResource (device, sizeof (uint32_t) * (kSubdivision_ * kSubdivision_) * 6);
	//indexBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&indexData_));
	//ibView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress ();
	//ibView_.SizeInBytes = UINT (sizeof (uint32_t) * (kSubdivision_ * kSubdivision_) * 6);
	//ibView_.Format = DXGI_FORMAT_R32_UINT;

	//行列データ
	matrixBuffer_ = CreateBufferResource (device, sizeof (TransformationMatrix));
	matrixBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&matrixData_));
	matrixData_->World = MakeIdentity4x4 ();
	matrixData_->WVP = MakeIdentity4x4 ();
	matrixData_->WorldInverseTranspose = MakeIdentity4x4 ();
	transform_ = {};
	uvTransform_ = {};
	transformationMatrix_.World = MakeIdentity4x4 ();
	transformationMatrix_.WVP = MakeIdentity4x4 ();
	transformationMatrix_.WorldInverseTranspose = MakeIdentity4x4 ();

	//マテリアルデータ
	materialBuffer_ = CreateBufferResource (device, sizeof (Material));
	materialBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&materialData_));
	materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData_->enableLighting = true;
	materialData_->uvTranform = MakeIdentity4x4 ();

	for (int i = 0; i < 4; i++) {
		color_[i] = 1.0f;
	}
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

void Model::Draw (ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle) {
	//どんな形状で描画するのか
	cmdList->IASetPrimitiveTopology (D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//頂点バッファをセットする
	cmdList->IASetVertexBuffers (0, 1, &vbView_);	//VBVを設定
	//定数バッファのルートパラメータを設定する	
	cmdList->SetGraphicsRootConstantBufferView (0, matrixBuffer_->GetGPUVirtualAddress ());
	cmdList->SetGraphicsRootConstantBufferView (1, materialBuffer_->GetGPUVirtualAddress ());
	//テクスチャのSRVを設定
	cmdList->SetGraphicsRootDescriptorTable (2, textureHandle);
	//実際に描画する(後々Index描画に変える)
	cmdList->DrawInstanced (static_cast<UINT>(model_.vertexCount), 1, 0, 0);
}

void Model::ImGui () {
	std::string label = "##" + id_;
	if (ImGui::ColorEdit4 (("Color" + label).c_str(), color_)) {
		// 色が変更されたらmaterialDataに反映
		materialData_->color.x = color_[0];
		materialData_->color.y = color_[1];
		materialData_->color.z = color_[2];
		materialData_->color.w = color_[3];
	}
	ImGui::DragFloat3 (("scale" + label).c_str(), &transform_.scale.x, 0.01f);
	ImGui::DragFloat3 (("rotate" + label).c_str(), &transform_.rotate.x, 0.01f);
	ImGui::DragFloat3 (("translate" + label).c_str(), &transform_.translate.x, 0.01f);
	ImGui::DragFloat3 (("UVscale" + label).c_str (), &uvTransform_.scale.x, 0.01f);
	ImGui::DragFloat3 (("UVrotate" + label).c_str (), &uvTransform_.rotate.x, 0.01f);
	ImGui::DragFloat3 (("UVtranslate" + label).c_str (), &uvTransform_.translate.x, 0.01f);
	//ライトの種類を選べるようにする
	static int currentNum = 1;
	const char* lights[] = { "None", "lambert", "halfLambert" };
	if(ImGui::Combo ("ライティング", &currentNum, lights, IM_ARRAYSIZE (lights))) {
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