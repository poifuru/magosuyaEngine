#include "SphereModel.h"
#include "../../header/pch.h"
#include "../utility/function.h"
#include "../utility/Math.h"

SphereModel::SphereModel (ID3D12Device* device, int subdivision) {
	//球の分割数を決める
	kSubdivision_ = subdivision;

	const int vertexCount = kSubdivision_ * kSubdivision_ * 6;
	vertexData_.resize (vertexCount); // ★ これ重要！

	//それぞれのバッファを作成してMapする、頂点バッファ・インデックスバッファのビューも設定しておく
	vertexBuffer_ = CreateBufferResource (device, sizeof (VertexData) * vertexCount);
	vertexBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&vertexDataPtr_));
	vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress ();
	vbView_.SizeInBytes = UINT (sizeof (VertexData) * vertexData_.size ());
	vbView_.StrideInBytes = sizeof (VertexData);

	//お前は今じゃない、また今度な！
	/*indexBuffer_ = CreateBufferResource (device, sizeof (uint32_t) * (kSubdivision_ * kSubdivision_) * 6);
	indexBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&indexData_));
	ibView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress ();
	ibView_.SizeInBytes = UINT (sizeof (uint32_t) * (kSubdivision_ * kSubdivision_) * 6);
	ibView_.Format = DXGI_FORMAT_R32_UINT;*/

	matrixBuffer_ = CreateBufferResource (device, sizeof (TransformationMatrix));
	matrixBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&matrixData_));
	matrixData_->World = MakeIdentity4x4 ();
	matrixData_->WVP = MakeIdentity4x4 ();
	matrixData_->WorldInverseTranspose = MakeIdentity4x4 ();

	materialBuffer_ = CreateBufferResource (device, sizeof (Material));
	materialBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&materialData_));
	materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData_->enableLighting = true;
	materialData_->uvTranform = MakeIdentity4x4 ();

	transform_ = {};
	uvTransform_ = {};
	transformationMatrix_.World = MakeIdentity4x4 ();
	transformationMatrix_.WVP = MakeIdentity4x4 ();
	radius_ = 1.0f;

	for (int i = 0; i < 4; i++) {
		color_[i] = 1.0f;
	}
}

SphereModel::~SphereModel () {
	if (vertexBuffer_) {
		vertexBuffer_->Unmap (0, nullptr);
		vertexBuffer_.Reset ();
	}
	if (matrixBuffer_) {
		matrixBuffer_->Unmap (0, nullptr);
		matrixBuffer_.Reset ();
	}
	if (materialBuffer_) {
		materialBuffer_->Unmap (0, nullptr);
		materialBuffer_.Reset ();
	}
}

void SphereModel::Initialize (Vector3 position, float radius) {
	transform_ = {
		{ 1.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f },
		position
	};
	uvTransform_ = {
		{ 1.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f }
	};
	radius_ = radius;

	const float kLonEvery = float (M_PI) * 2.0f / float (kSubdivision_);
	const float kLatEvery = float (M_PI) / float (kSubdivision_);

	const int vertexCount = kSubdivision_ * kSubdivision_ * 6;
	vertexData_.resize (vertexCount); // ★ これ重要！

	uint32_t index = 0;

	for (int latIndex = 0; latIndex < kSubdivision_; ++latIndex) {
		float lat = -float (M_PI) / 2.0f + kLatEvery * latIndex;
		float latNext = lat + kLatEvery;

		for (int lonIndex = 0; lonIndex < kSubdivision_; ++lonIndex) {
			float lon = lonIndex * kLonEvery;
			float lonNext = lon + kLonEvery;

			Vector3 a = {
				radius * cosf (lat) * cosf (lon),
				radius * sinf (lat),
				radius * cosf (lat) * sinf (lon)
			};
			Vector3 b = {
				radius * cosf (latNext) * cosf (lon),
				radius * sinf (latNext),
				radius * cosf (latNext) * sinf (lon)
			};
			Vector3 c = {
				radius * cosf (lat) * cosf (lonNext),
				radius * sinf (lat),
				radius * cosf (lat) * sinf (lonNext)
			};
			Vector3 d = {
				radius * cosf (latNext) * cosf (lonNext),
				radius * sinf (latNext),
				radius * cosf (latNext) * sinf (lonNext)
			};

			//texcoordをしっかり書き込むための変数
			float u = float (lonIndex) / float (kSubdivision_);
			float uNext = float (lonIndex + 1) / float (kSubdivision_);
			float v = 1.0f - float (latIndex) / float (kSubdivision_);
			float vNext = 1.0f - float (latIndex + 1) / float (kSubdivision_);

			// 三角形1: a, b, d
			vertexData_[index].position = Vector4 (a.x, a.y, a.z, 1.0f);
			vertexData_[index].normal = Normalize (a);
			vertexData_[index].texcoord = { u, v };
			//indexData_[index] = index;
			index++;

			vertexData_[index].position = Vector4 (b.x, b.y, b.z, 1.0f);
			vertexData_[index].normal = Normalize (b);
			vertexData_[index].texcoord = { u, vNext };
			//indexData_[index] = index;
			index++;

			vertexData_[index].position = Vector4 (d.x, d.y, d.z, 1.0f);
			vertexData_[index].normal = Normalize (d);
			vertexData_[index].texcoord = { uNext, vNext };
			//indexData_[index] = index;
			index++;

			// 三角形2: a, d, c
			vertexData_[index].position = Vector4 (a.x, a.y, a.z, 1.0f);
			vertexData_[index].normal = Normalize (a);
			vertexData_[index].texcoord = { u, v };
			//indexData_[index] = index;
			index++;

			vertexData_[index].position = Vector4 (d.x, d.y, d.z, 1.0f);
			vertexData_[index].normal = Normalize (d);
			vertexData_[index].texcoord = { uNext, vNext };
			//indexData_[index] = index;
			index++;

			vertexData_[index].position = Vector4 (c.x, c.y, c.z, 1.0f);
			vertexData_[index].normal = Normalize (c);
			vertexData_[index].texcoord = { uNext, v };
			//indexData_[index] = index;
			index++;
		}
	}

	/*for (int i = 0; i < 6; ++i) {
		char buffer[128];
		sprintf_s (buffer, "uv[%d] = (%f, %f)\n", i, vertexData_[i].texcoord.x, vertexData_[i].texcoord.y);
		OutputDebugStringA (buffer);
	}*/
	//GPUに渡すために手動でコピーする
	memcpy (vertexDataPtr_, vertexData_.data (), sizeof (VertexData)* vertexCount);
}

void SphereModel::Update (Matrix4x4* view, Matrix4x4* proj) {
	transformationMatrix_.World = MakeAffineMatrix (transform_.scale, transform_.rotate, transform_.translate);
	transformationMatrix_.WVP = Multiply (transformationMatrix_.World, Multiply (*view, *proj));

	matrixData_->World = transformationMatrix_.World;
	matrixData_->WVP = transformationMatrix_.WVP;
	matrixData_->WorldInverseTranspose = Transpose (Inverse (matrixData_->World));

	//uvTranform更新
	materialData_->uvTranform = MakeAffineMatrix (uvTransform_.scale, uvTransform_.rotate, uvTransform_.translate);
}

void SphereModel::Draw (ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle) {
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
	cmdList->DrawInstanced ((kSubdivision_ * kSubdivision_ * 6), 1, 0, 0);
}

void SphereModel::ShowImGuiEditor () {
	if (ImGui::ColorEdit4 ("Color##sphere", color_)) {
		// 色が変更されたらmaterialDataに反映
		materialData_->color.x = color_[0];
		materialData_->color.y = color_[1];
		materialData_->color.z = color_[2];
		materialData_->color.w = color_[3];
	}
	ImGui::DragFloat3 ("scale##sphere", &transform_.scale.x, 0.01f);
	ImGui::DragFloat3 ("rotate##sphere", &transform_.rotate.x, 0.01f);
	ImGui::DragFloat3 ("translate##sphere", &transform_.translate.x, 0.01f);
	ImGui::Separator ();
}