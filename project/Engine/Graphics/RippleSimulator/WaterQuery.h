#pragma once
#include "ConstantBuffer.h"
#include <bitset>

namespace MyEngine::LowLevel {
	class SrvDescriptorHeapPool;
}

namespace MyEngine::Rendering {
	class ShaderManager;
}

struct WaterQueryParams {
	static constexpr uint32_t kMaxQueries = 64; // WaterQuery.CS.hlsl の kMaxQueries と合わせる
	Vector4 points[kMaxQueries]; // x, y にワールドの (x, z)
	Vector2 waterMin;
	Vector2 waterSize;
	uint32_t rippleTextureIndex;
	uint32_t count;
	float padding[2];
};

// 1点ぶんの結果
struct WaterQueryResult {
	float height = 0.0f;  // 波紋の高さ[m]
	float dhdx = 0.0f;    // x 方向の傾き
	float dhdz = 0.0f;    // z 方向の傾き
	float inRange = 0.0f; // シミュ範囲内なら 1
};

class WaterQuery {
public:
	static constexpr uint32_t kMaxQueries = WaterQueryParams::kMaxQueries;

	// --- 枠の割り当て ---
	// 船とボーダーは決まった枠を使う。それ以外（木箱・ウキ・アイテムなど）は AllocateSlot で空き枠を借りる
	static constexpr uint32_t kBoatSlot = 0;                 // 船の後ろ（BoatWakeComponent）
	static constexpr uint32_t kBoarderSlot = kMaxQueries - 1; // ボーダー

	// 空いている枠を借りる（空きが無ければ -1）。使い終わったら FreeSlot で返す
	// ※ 枠の管理は static なので、WaterQuery 本体より先に・後に壊れても安全
	static int32_t AllocateSlot();
	static void FreeSlot(int32_t slot);
	static uint32_t GetAllocatedSlotCount();

	void Initialize(
		ID3D12Device* device,
		MyEngine::LowLevel::SrvDescriptorHeapPool& heapManager,
		MyEngine::Rendering::ShaderManager& shaderManager,
		ID3D12RootSignature* computeRootSignature
	);

	// Update 中に呼ぶ：slot 番目に調べたい点を登録
	void SetQueryPoint(uint32_t slot, float x, float z);

	// Update 中に呼ぶ：前フレームに登録した点の結果（1フレーム遅れ）
	const WaterQueryResult& GetResult(uint32_t slot);

	// 描画中に呼ぶ：波紋CSの直後
	void Dispatch(ID3D12GraphicsCommandList* cmdList, uint32_t rippleSrvIndex,
				  const Vector2& waterMin, const Vector2& waterSize);

private:
	void ReadBackIfNeeded();

	Microsoft::WRL::ComPtr<ID3D12Resource> resultBuffer_;   // GPUが書く
	Microsoft::WRL::ComPtr<ID3D12Resource> readbackBuffer_; // CPUが読む
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;
	ID3D12RootSignature* rootSig_ = nullptr;
	MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager_ = nullptr;
	uint32_t uavIndex_ = 0;

	ConstantBuffer<WaterQueryParams> paramBuffer_;
	WaterQueryParams params_{};
	uint32_t usedSlots_ = 0;      // 登録された一番大きい slot + 1

	WaterQueryResult results_[kMaxQueries]{};
	bool hasPendingCopy_ = false; // GPUにコピーを積んだが、まだ読んでいない

	// 使用中の枠（AllocateSlot で借りた枠）
	static std::bitset<kMaxQueries>& UsedSlots();
};