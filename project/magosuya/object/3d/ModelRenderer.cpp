#include "ModelRenderer.h"
#include <imgui.h>
#include "mathFunction.h"
#include "DxCommon.h"

ModelRenderer::ModelRenderer (DxCommon* dxCommon) {
	dxCommon_ = dxCommon;
	commandList_ = dxCommon->GetCommandList ();
	for (int i = 0; i < 4; ++i) {
		color_[i] = 1.0f;
	}
}

ModelRenderer::~ModelRenderer () {
}

void ModelRenderer::Initialize () {
	//===リソースの初期化===//
	//行列データ
	matrixBuffer_ = dxCommon_->CreateBufferResource (sizeof (TransformationMatrix));
	matrixBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&matrixData_));
	matrixData_->World = MakeIdentity4x4 ();
	matrixData_->WVP = MakeIdentity4x4 ();
	matrixData_->WorldInverseTranspose = MakeIdentity4x4 ();

	//マテリアルデータ
	materialBuffer_ = dxCommon_->CreateBufferResource (sizeof (Material));
	materialBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&materialData_));
	materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData_->enableLighting = true;
	materialData_->uvTranform = MakeIdentity4x4 ();
}

void ModelRenderer::Update (Matrix4x4 world, Matrix4x4 vp, Transform uvTransform) {
	matrixData_->World = world;
	matrixData_->WVP = Multiply(matrixData_->World, vp);
	matrixData_->WorldInverseTranspose = Transpose (Inverse (matrixData_->World));

	//uvTranform更新
	materialData_->uvTranform = MakeAffineMatrix (uvTransform.scale, uvTransform.rotate, uvTransform.translate);
}

void ModelRenderer::Draw (D3D12_GPU_DESCRIPTOR_HANDLE textureHandle) {
	// 共有データをロックして有効性をチェック
	std::shared_ptr<ModelData> data = modelData_.lock ();
	if (!data) {
		// モデルデータが解放済みなら描画をスキップ
		return;
	}
	//どんな形状で描画するのか
	commandList_->IASetPrimitiveTopology (D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//頂点バッファをセットする
	commandList_->IASetVertexBuffers (0, 1, &data->vbView);
	//インデックスバッファをセットする
	commandList_->IASetIndexBuffer (&data->ibView);
	//定数バッファのルートパラメータを設定する	
	commandList_->SetGraphicsRootConstantBufferView (0, matrixBuffer_->GetGPUVirtualAddress ());
	commandList_->SetGraphicsRootConstantBufferView (1, materialBuffer_->GetGPUVirtualAddress ());
	//テクスチャのSRVを設定
	commandList_->SetGraphicsRootDescriptorTable (2, textureHandle);
	//実際に描画する(後々Index描画に変える)
	commandList_->DrawIndexedInstanced (static_cast<UINT>(data->indexCount), 1, 0, 0, 0);
}

void ModelRenderer::ImGui (Transform& transform, Transform& uvTransform) {
	std::string label = "##" + tag_;
	if (ImGui::ColorEdit4 (("Color" + label).c_str (), color_)) {
		// 色が変更されたらmaterialDataに反映
		materialData_->color.x = color_[0];
		materialData_->color.y = color_[1];
		materialData_->color.z = color_[2];
		materialData_->color.w = color_[3];
	}
	ImGui::DragFloat3 (("scale" + label).c_str (), &transform.scale.x, 0.01f);
	ImGui::DragFloat3 (("rotate" + label).c_str (), &transform.rotate.x, 0.01f);
	ImGui::DragFloat3 (("translate" + label).c_str (), &transform.translate.x, 0.01f);
	ImGui::DragFloat3 (("UVscale" + label).c_str (), &uvTransform.scale.x, 0.01f);
	ImGui::DragFloat3 (("UVrotate" + label).c_str (), &uvTransform.rotate.x, 0.01f);
	ImGui::DragFloat3 (("UVtranslate" + label).c_str (), &uvTransform.translate.x, 0.01f);
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