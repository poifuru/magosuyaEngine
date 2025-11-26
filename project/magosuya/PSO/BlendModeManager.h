#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <array>

enum class BlendModeType : uint8_t {
	Opaque,         // 不透明
	Alpha,          // アルファブレンド
	Additive,       // 加算合成
	Subtract,       // 減算合成
	Multiply,       // 乗算合成
	Screen,         // スクリーン合成
	
	Count			// モードの個数
};

const int kBlendDescNum = (int)BlendModeType::Count;

class BlendModeManager {
public:		//メンバ関数
	static BlendModeManager* GetInstance() {
		//初めて呼び出されたときに一回だけ初期化
		static BlendModeManager instance;
		return &instance;
	}

	void Initialize ();

	//BlectModeTypeを受け取って、対応するブレンド設定を返す
	const D3D12_BLEND_DESC& GetBlendDesc (BlendModeType type) const;

private:	//メンバ変数
	//ブレンド設定の配列
	std::array< D3D12_BLEND_DESC, kBlendDescNum> m_BlendDescs;

};