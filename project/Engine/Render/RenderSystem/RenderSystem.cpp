#include "PCH.h"
#include "RenderSystem.h"
#include "PSOManager.h"
#include "SrvDescriptorHeapPool.h"
#include "ShaderManager.h"
#include "LightManager.h"

void MyEngine::Rendering::RenderSystem::Initialize(
	ID3D12Device* device,
	MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager,
	PSOManager* psoManager,
	const ShaderManager* shaderManager,
	const InputLayoutManager* inputLayoutManager,
	const BlendModeManager* blendModeManager,
	ID3D12RootSignature* commonRootSignature
) {
	assert(device != nullptr);
	device_ = device;
	heapManager_ = heapManager;
	psoManager_ = psoManager;
	shaderManager_ = shaderManager;
	inputLayoutManager_ = inputLayoutManager;
	blendModeManager_ = blendModeManager;
	commonRootSignature_ = commonRootSignature;
	cameraBuffer_.Initialize(device);
}

void MyEngine::Rendering::RenderSystem::PushCommand(const RenderCommand& command) {
	commandQueue_.push_back(command);
}

void MyEngine::Rendering::RenderSystem::SetCameraPosition(const Vector3& cameraPos) {
	CameraForGPU gpuData{};
	gpuData.worldPosition = cameraPos;
	gpuData.padding = 0.0f;
	cameraBuffer_.Update(gpuData);
}

uint32_t MyEngine::Rendering::RenderSystem::sDrawCallCount_ = 0;

void MyEngine::Rendering::RenderSystem::SetLightManager(LightManager* lightManager) {
	activeLightManager_ = lightManager;
}

void MyEngine::Rendering::RenderSystem::WriteCommandList(ID3D12GraphicsCommandList* cmdList) {
	// ドローコール数を記録（コマンドキューのサイズ＝ドローコール数）
	sDrawCallCount_ = static_cast<uint32_t>(commandQueue_.size());

	// レイヤー順（不透明・半透明）にソート
	std::sort(commandQueue_.begin(), commandQueue_.end(), [](const RenderCommand& a, const RenderCommand& b) {
		return a.layer < b.layer;
	});

	// 共通ルートシグネチャをセット（フレームで1回固定）
	cmdList->SetGraphicsRootSignature(commonRootSignature_);

	// 巨大ディスクリプタヒープをステージング
	heapManager_->SetGraphicsHeap(cmdList);

	// Slot 2 と Slot 3 に、巨大ヒープの「先頭のGPUハンドル」をセット
	cmdList->SetGraphicsRootDescriptorTable(2, heapManager_->GetGpuHandle(0));
	cmdList->SetGraphicsRootDescriptorTable(3, heapManager_->GetGpuHandle(0));

	if (commandQueue_.empty()) return;

	// Slot 4: カメラバッファをバインド
	cmdList->SetGraphicsRootConstantBufferView(4, cameraBuffer_.GetGPUVirtualAddress());

	// Slot 5: ライトデータをバインド
	if (activeLightManager_ != nullptr) {
		cmdList->SetGraphicsRootConstantBufferView(5, activeLightManager_->GetLightGPUAddress());
	}

	// トポロジーは三角形
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 重複するPSOをスキップする最適化のためのキャッシュ変数
	ID3D12PipelineState* lastPSO = nullptr;

	// コマンド実行ループ
	for (const auto& cmd : commandQueue_) {

		// 直前と同じパイプライン（PSO）なら、切り替え（SetPipelineState）をスキップ
		ID3D12PipelineState* currentPSO = cmd.pso;

		if (currentPSO && currentPSO != lastPSO) {
			cmdList->SetPipelineState(currentPSO);
			lastPSO = currentPSO;
		}

		// ドローコールの前にイベントを開始
		PIXBeginEvent(cmdList, PIX_COLOR(100, 150, 255), cmd.debugName);

		// メッシュのバインド
		cmdList->IASetVertexBuffers(0, 1, &cmd.vbView);
		cmdList->IASetIndexBuffer(&cmd.ibv);

		// Slot 0: オブジェクト個別のトランスフォームバッファをバインド
		cmdList->SetGraphicsRootConstantBufferView(0, cmd.transformGPUAddress);

		// slot 6: カスタムバッファのアドレスがあればバインド
		if (cmd.customBufferGPUAddress != 0) {
			cmdList->SetGraphicsRootConstantBufferView(6, cmd.customBufferGPUAddress);
		}
		else {
			// 無ければダミーとしてtransformBufferをセットしてあげる
			cmdList->SetGraphicsRootConstantBufferView(6, cmd.transformGPUAddress);
		}

		// Slot 1: マテリアルとテクスチャのバインドレスインデックス
		uint32_t indices[2] = { cmd.materialIndex, cmd.textureIndex };
		cmdList->SetGraphicsRoot32BitConstants(1, 2, indices, 0);

		// ドローコール
		// indexCount がある場合はインデックス描画、無ければ頂点数で描画
		if (cmd.indexCount > 0) {
			cmdList->DrawIndexedInstanced(cmd.indexCount, 1, 0, 0, 0);
		} else {
			cmdList->DrawInstanced(cmd.vertexCount, 1, 0, 0);
		}

		// ドローコールが終わったら即座に閉じる！
		PIXEndEvent(cmdList);
	}
}

void MyEngine::Rendering::RenderSystem::ClearCommands() {
	commandQueue_.clear();
	activeLightManager_ = nullptr;
}