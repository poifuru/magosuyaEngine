#include "PCH.h"
#include "ShadowPass.h"
#include "RenderTargetFactory.h"
#include "DepthTexture.h"
#include "MathFunction.h"
#include "PSOManager.h"
#include "RenderSystem.h"

MyEngine::Rendering::ShadowPass::ShadowPass(
	ID3D12Device* device,
	LowLevel::DescriptorHeapPoolContext* heapContext
) {
	shadowMap_ =
		MyEngine::LowLevel::RenderTargetFactory::CreateDepthTexture(
			device,
			heapContext,
			kShadowMapSize,
			kShadowMapSize
		);
}

MyEngine::Rendering::ShadowPass::~ShadowPass() {
}

void MyEngine::Rendering::ShadowPass::Update(const Vector3& lightDir, const Vector3& center) {
	const float distance = 100.0f;	// ライトカメラを中心からどれだけ離すか

	Vector3 dir = Math::Normalize(lightDir);
	Vector3 eye = center - dir * distance;

	// ライトが真上/真下を向くとupと平行になるので別のupにする
	Vector3 up = (std::abs(dir.y) > 0.99f) ? Vector3{ 0.0f, 0.0f, 1.0f } : Vector3{ 0.0f, 1.0f, 0.0f };

	Matrix4x4 view = Math::MakeLookAtMatrix(eye, center, up);
	Matrix4x4 proj = Math::MakeOrthographicMatrix(-range_, range_, range_, -range_, 0.1f, distance * 2.0f);
	lightViewProj_ = Math::Multiply(view, proj);
}

void MyEngine::Rendering::ShadowPass::Execute(ID3D12GraphicsCommandList* cmdList, RenderSystem* renderSystem) {
	// 深度を書き込める状態にする（前フレームのメインパスでSRVとして読んでいるため）
	shadowMap_->ChangeState(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	// RTVは無し、DSVだけをシャドウマップにする
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = shadowMap_->GetDescriptorHandle();
	cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
	cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// ビューポートとシザーをシャドウマップのサイズにする（メインパスの前に画面サイズへ戻すこと）
	D3D12_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(kShadowMapSize);
	viewport.Height = static_cast<float>(kShadowMapSize);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	cmdList->RSSetViewports(1, &viewport);

	D3D12_RECT scissorRect{};
	scissorRect.right = static_cast<LONG>(kShadowMapSize);
	scissorRect.bottom = static_cast<LONG>(kShadowMapSize);
	cmdList->RSSetScissorRects(1, &scissorRect);

	renderSystem->WriteShadowCommandList(cmdList);

	// メインパスのPSで読めるようにする
	shadowMap_->ChangeState(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void MyEngine::Rendering::ShadowPass::ApplySettings(PSODescriptor& desc) const {
	// 深度だけ書くのでPSもRTVも使わない
	desc.UsePixelShader = false;
	desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.CullMode = cullMode_;
}

void MyEngine::Rendering::ShadowPass::ImGui() {
#ifdef USEIMGUI
	ImGui::DragFloat("範囲", &range_, 0.5f, 1.0f, 500.0f);
	ImGui::DragFloat("バイアス", &bias_, 0.0001f, 0.0f, 0.05f, "%.4f");

	// 物体の形が白黒で見えればシャドウパスは成功（真っ白なら何も描かれていない）
	ImGui::Image((ImTextureID)shadowMap_->GetSrvGpuHandle().ptr, ImVec2(256.0f, 256.0f));
#endif
}

const Matrix4x4& MyEngine::Rendering::ShadowPass::GetLightViewProj() const {
	return lightViewProj_;
}

uint32_t MyEngine::Rendering::ShadowPass::GetShadowMapSrvIndex() const {
	return shadowMap_->GetSrvIndex();
}

float MyEngine::Rendering::ShadowPass::GetBias() const {
	return bias_;
}
