#include "PCH.h"
#include "LineRenderer.h"
#include "PSOManager.h"
#include "ShaderManager.h"
#include "InputLayoutManager.h"
#include "BlendModeManager.h"
#include "RenderingModel.h"
#include <cmath>

void LineRenderer::Initialize(
	ID3D12Device* device,
	MyEngine::Rendering::PSOManager* psoManager,
	MyEngine::Rendering::ShaderManager* shaderManager,
	const MyEngine::Rendering::InputLayoutManager* inputLayoutManager,
	const MyEngine::Rendering::BlendModeManager* blendModeManager,
	ID3D12RootSignature* commonRootSignature
) {
	assert(device && psoManager && shaderManager && inputLayoutManager && blendModeManager && commonRootSignature);
	rootSignature_ = commonRootSignature;

	// --- 頂点バッファ（UPLOAD ヒープ、Map しっぱなし） ---
	const UINT vertexCount = kMaxLines * 2 * 2; // 線1本 = 2頂点、深度あり・なしの2区画
	const UINT bufferSize = sizeof(LineVertex) * vertexCount;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = bufferSize;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(vertexBuffer_.GetAddressOf())
	);
	assert(SUCCEEDED(hr));
	vertexBuffer_->SetName(L"LineRenderer::VertexBuffer");

	// CPU は書くだけなので読み取り範囲は空
	D3D12_RANGE readRange{ 0, 0 };
	hr = vertexBuffer_->Map(0, &readRange, reinterpret_cast<void**>(&mappedVertices_));
	assert(SUCCEEDED(hr));

	vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vbView_.SizeInBytes = bufferSize;
	vbView_.StrideInBytes = sizeof(LineVertex);

	// --- ViewProjection の定数バッファ ---
	viewProjBuffer_.Initialize(device);

	// --- PSO ---
	MyEngine::Rendering::PSODescriptor desc{};
	desc.VS_ID = shaderManager->CompileAndCacheShader(L"Resources/Shader/line.VS.hlsl", L"vs_6_0");
	desc.PS_ID = shaderManager->CompileAndCacheShader(L"Resources/Shader/line.PS.hlsl", L"ps_6_0");
	desc.InputLayoutID = MyEngine::Rendering::InputLayoutType::LineMesh;
	desc.BlendMode = MyEngine::Rendering::BlendModeType::Alpha;       // 半透明の線も描けるように
	desc.CullMode = D3D12_CULL_MODE_NONE;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE; // 線として組み立てる
	desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;               // 線は深度を書かない

	// 深度テストあり（物の後ろは隠れる）
	desc.DepthEnable = TRUE;
	psoDepth_ = psoManager->GetOrCreatePSO(device, desc, commonRootSignature, *shaderManager, *inputLayoutManager, *blendModeManager);

	// 深度テストなし（常に手前）
	desc.DepthEnable = FALSE;
	psoOverlay_ = psoManager->GetOrCreatePSO(device, desc, commonRootSignature, *shaderManager, *inputLayoutManager, *blendModeManager);

	depthCount_ = 0;
	overlayCount_ = 0;
	isInitialized_ = true;
}

void LineRenderer::Finalize() {
	if (vertexBuffer_ && mappedVertices_) {
		vertexBuffer_->Unmap(0, nullptr);
	}
	mappedVertices_ = nullptr;
	vertexBuffer_.Reset();
	viewProjBuffer_.Release();
	psoDepth_ = nullptr;
	psoOverlay_ = nullptr;
	rootSignature_ = nullptr;
	depthCount_ = 0;
	overlayCount_ = 0;
	isInitialized_ = false;
}

void LineRenderer::DrawLine(const Vector3& a, const Vector3& b, const Vector4& color, bool depthTest) {
	if (!isInitialized_ || !isEnabled_) return;

	// 区画ごとの本数と書き込み先を選ぶ
	uint32_t& count = depthTest ? depthCount_ : overlayCount_;
	if (count >= kMaxLines) return; // あふれたら捨てる

	uint32_t base = (depthTest ? 0u : kMaxLines * 2) + count * 2;
	mappedVertices_[base + 0] = { a, color };
	mappedVertices_[base + 1] = { b, color };
	count++;
}

void LineRenderer::DrawCircleXZ(const Vector3& center, float radius, const Vector4& color, int segments, bool depthTest) {
	if (segments < 3) segments = 3;
	const float kTwoPi = 6.2831853f;

	Vector3 prev = { center.x + radius, center.y, center.z };
	for (int i = 1; i <= segments; ++i) {
		float t = kTwoPi * static_cast<float>(i) / static_cast<float>(segments);
		Vector3 cur = { center.x + std::cos(t) * radius, center.y, center.z + std::sin(t) * radius };
		DrawLine(prev, cur, color, depthTest);
		prev = cur;
	}
}

void LineRenderer::DrawArrow(const Vector3& from, const Vector3& to, const Vector4& color, float headRatio, bool depthTest) {
	DrawLine(from, to, color, depthTest);

	Vector3 d = { to.x - from.x, to.y - from.y, to.z - from.z };
	float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
	if (len < 1e-5f) return;
	d = { d.x / len, d.y / len, d.z / len };

	// 矢印の向きに垂直な軸（真上向きの矢印なら X 軸を使う）
	Vector3 up = (std::abs(d.y) > 0.99f) ? Vector3{ 1.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 1.0f, 0.0f };
	Vector3 side = { d.y * up.z - d.z * up.y, d.z * up.x - d.x * up.z, d.x * up.y - d.y * up.x }; // d × up
	float sideLen = std::sqrt(side.x * side.x + side.y * side.y + side.z * side.z);
	side = { side.x / sideLen, side.y / sideLen, side.z / sideLen };

	float head = len * headRatio;
	Vector3 back = { to.x - d.x * head, to.y - d.y * head, to.z - d.z * head };
	float w = head * 0.5f;
	DrawLine(to, { back.x + side.x * w, back.y + side.y * w, back.z + side.z * w }, color, depthTest);
	DrawLine(to, { back.x - side.x * w, back.y - side.y * w, back.z - side.z * w }, color, depthTest);
}

void LineRenderer::DrawGridXZ(const Vector3& center, float size, float step, const Vector4& color, bool depthTest) {
	if (step <= 0.0f || size <= 0.0f) return;
	float half = size * 0.5f;
	int lineCount = static_cast<int>(size / step);

	for (int i = 0; i <= lineCount; ++i) {
		float offset = -half + step * static_cast<float>(i);
		// Z方向の線
		DrawLine({ center.x + offset, center.y, center.z - half }, { center.x + offset, center.y, center.z + half }, color, depthTest);
		// X方向の線
		DrawLine({ center.x - half, center.y, center.z + offset }, { center.x + half, center.y, center.z + offset }, color, depthTest);
	}
}

void LineRenderer::DrawAxis(const Vector3& origin, float length, bool depthTest) {
	DrawLine(origin, { origin.x + length, origin.y, origin.z }, { 1.0f, 0.0f, 0.0f, 1.0f }, depthTest);
	DrawLine(origin, { origin.x, origin.y + length, origin.z }, { 0.0f, 1.0f, 0.0f, 1.0f }, depthTest);
	DrawLine(origin, { origin.x, origin.y, origin.z + length }, { 0.0f, 0.0f, 1.0f, 1.0f }, depthTest);
}

void LineRenderer::Render(ID3D12GraphicsCommandList* cmdList, const Matrix4x4& viewProj) {
	if (!isInitialized_ || !cmdList) return;
	if (depthCount_ == 0 && overlayCount_ == 0) return;

	viewProjBuffer_.Update(viewProj);

	PIXBeginEvent(cmdList, PIX_COLOR(255, 200, 0), "LineRenderer");

	// 共通ルートシグネチャの Slot 0（b0）に ViewProjection を入れる
	cmdList->SetGraphicsRootSignature(rootSignature_);
	cmdList->SetGraphicsRootConstantBufferView(0, viewProjBuffer_.GetGPUVirtualAddress());

	// PSO 側の TOPOLOGY_TYPE_LINE と、こちらの LINELIST は両方そろえる必要がある
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView_);

	// 深度テストあり：バッファの前半
	if (depthCount_ > 0) {
		cmdList->SetPipelineState(psoDepth_);
		cmdList->DrawInstanced(depthCount_ * 2, 1, 0, 0);
	}

	// 常に手前：バッファの後半（StartVertexLocation で後半の先頭から読む）
	if (overlayCount_ > 0) {
		cmdList->SetPipelineState(psoOverlay_);
		cmdList->DrawInstanced(overlayCount_ * 2, 1, kMaxLines * 2, 0);
	}

	PIXEndEvent(cmdList);
}

void LineRenderer::Clear() {
	depthCount_ = 0;
	overlayCount_ = 0;
}
