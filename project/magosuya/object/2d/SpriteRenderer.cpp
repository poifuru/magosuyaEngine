#include "SpriteRenderer.h"
#include <imgui.h>
#include <DirectXTex.h>
#include "MathFunction.h"
#include "MagosuyaEngine.h"

SpriteRenderer::SpriteRenderer (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
	rootSignature_ = magosuya_->GetDxCommon()->GetRootSignature ();
	pipelineState_ = magosuya_->GetDxCommon ()->GetPipelineState ();

	for (int i = 0; i < 4; i++) {
		color_[i] = 1.0f;
	}
}

SpriteRenderer::~SpriteRenderer () {
}

void SpriteRenderer::Initialize () {
	//頂点・インデックスバッファ作成 → Mapして vertexData_, indexData_ に保持
	//それぞれビューの設定も
	vertexBuffer_ = magosuya_->GetDxCommon ()->CreateBufferResource (sizeof (VertexData) * 4);
	vertexBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&vertexData_));
	vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress ();
	vbView_.SizeInBytes = sizeof (VertexData) * 4;
	vbView_.StrideInBytes = sizeof (VertexData);

	indexBuffer_ = magosuya_->GetDxCommon ()->CreateBufferResource (sizeof (uint32_t) * 6);
	indexBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&indexData_));
	ibView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress ();
	ibView_.SizeInBytes = sizeof (uint32_t) * 6;
	ibView_.Format = DXGI_FORMAT_R32_UINT;

	//行列・マテリアル用の定数バッファも作成 → Mapして material_, wvpMatrix_ に保持
	//初期設定まで済ませる
	matrixBuffer_ = magosuya_->GetDxCommon ()->CreateBufferResource ((sizeof (Matrix4x4) + 255) & ~255);
	matrixBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&matrixData_));
	*matrixData_ = MakeIdentity4x4 ();

	materialBuffer_ = magosuya_->GetDxCommon ()->CreateBufferResource (sizeof (Material));
	materialBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&materialData_));
	materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };	//初期カラーは白
	materialData_->enableLighting = false;
	materialData_->uvTranform = MakeIdentity4x4 ();

	//indexData_に書き込み
	indexData_[0] = 0;
	indexData_[1] = 1;
	indexData_[2] = 2;
	indexData_[3] = 1;
	indexData_[4] = 3;
	indexData_[5] = 2;
}

void SpriteRenderer::Update (Matrix4x4 wvpData, Transform uvTransform, Vector2 anchorPoint, bool flipX, bool flipY, const std::string& id, Vector2 texLeftTop, Vector2 texSize) {
	//vertexData_に初期の四角形座標を書く（size_を使って計算）
	float left = 0.0f - anchorPoint.x;
	float right = 1.0f - anchorPoint.x;
	float top = 0.0f - anchorPoint.y;
	float bottom = 1.0f - anchorPoint.y;

	//画像切り出し用
	id_ = id;
	const DirectX::TexMetadata& metadata = magosuya_->GetTextureManger()->GetMetaData (id_);
	float tex_left = texLeftTop.x / metadata.width;
	float tex_right = (texLeftTop.x + texSize.x) / metadata.width;
	float tex_top = texLeftTop.y / metadata.height;
	float tex_bottom = (texLeftTop.y + texSize.y) / metadata.height;

	if (flipX) {
		left = -left;
		right = -right;
	}
	if (flipY) {
		top = -top;
		bottom = -bottom;
	}

	vertexData_[0].position = { left, bottom, 0.0f, 1.0f };      //左下
	vertexData_[0].texcoord = { tex_left, tex_bottom };
	vertexData_[0].normal = { 0.0f, 0.0f, -1.0f };

	vertexData_[1].position = { left, top, 0.0f, 1.0f };   //左上
	vertexData_[1].texcoord = { tex_left, tex_top };
	vertexData_[1].normal = { 0.0f, 0.0f, -1.0f };

	vertexData_[2].position = { right, bottom, 0.0f, 1.0f };         //右下
	vertexData_[2].texcoord = { tex_right, tex_bottom };
	vertexData_[2].normal = { 0.0f, 0.0f, -1.0f };

	vertexData_[3].position = { right, top, 0.0f, 1.0f };      //右上
	vertexData_[3].texcoord = { tex_right, tex_top };
	vertexData_[3].normal = { 0.0f, 0.0f, -1.0f };

	*matrixData_ = wvpData;
	// UVTransform更新
	materialData_->uvTranform = MakeAffineMatrix (uvTransform.scale, uvTransform.rotate, uvTransform.translate);
}

void SpriteRenderer::Draw (D3D12_GPU_DESCRIPTOR_HANDLE textureHandle) {
	magosuya_->GetDxCommon ()->GetCommandList ()->SetGraphicsRootSignature (rootSignature_.Get());
	magosuya_->GetDxCommon ()->GetCommandList ()->SetPipelineState (pipelineState_.Get());
	magosuya_->GetDxCommon ()->GetCommandList ()->IASetPrimitiveTopology (D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	magosuya_->GetDxCommon ()->GetCommandList ()->IASetVertexBuffers (0, 1, &vbView_);   //VBVを設定
	magosuya_->GetDxCommon ()->GetCommandList ()->IASetIndexBuffer (&ibView_);	        //IBVを設定
	magosuya_->GetDxCommon ()->GetCommandList ()->SetGraphicsRootConstantBufferView (0, matrixBuffer_->GetGPUVirtualAddress ());
	magosuya_->GetDxCommon ()->GetCommandList ()->SetGraphicsRootConstantBufferView (1, materialBuffer_->GetGPUVirtualAddress ());
	magosuya_->GetDxCommon ()->GetCommandList ()->SetGraphicsRootDescriptorTable (2, textureHandle);
	//こいつでインデックスバッファを使った描画
	magosuya_->GetDxCommon ()->GetCommandList ()->DrawIndexedInstanced (6, 1, 0, 0, 0);
}

void SpriteRenderer::ImGui (Transform& transform, Transform& uvTransform) {
	if (ImGui::ColorEdit4 ("Color##SpriteColor", color_)) {
		// 色が変更されたらmaterialDataに反映
		materialData_->color.x = color_[0];
		materialData_->color.y = color_[1];
		materialData_->color.z = color_[2];
		materialData_->color.w = color_[3];
	}
	ImGui::DragFloat3 ("Scale", &transform.scale.x, 1.0f);
	ImGui::DragFloat3 ("Rotate", &transform.rotate.x, 0.01f);
	ImGui::DragFloat3 ("Translate", &transform.translate.x, 1.0f);
	ImGui::DragFloat2 ("UVScale", &uvTransform.scale.x, 0.01f);
	ImGui::DragFloat ("UVRotate", &uvTransform.rotate.z, 0.01f);
	ImGui::DragFloat2 ("UVTranslate", &uvTransform.translate.x, 0.01f);
	ImGui::Separator ();
}
