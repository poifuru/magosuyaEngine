#include "BlendModeManager.h"
#include <cassert>

BlendModeManager::BlendModeManager () {
	//===ブレンド設定を全て初期化する===//
	//すべての色の要素を書き込む
	m_BlendDescs[0].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//共通設定なのでfor文で回す
	for (int i = 1; i < kBlendDescNum; i++) {
		m_BlendDescs[i].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		m_BlendDescs[i].RenderTarget[0].BlendEnable = TRUE;
		m_BlendDescs[i].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		m_BlendDescs[i].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		m_BlendDescs[i].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	}

	//---個別の設定---//
	//アルファブレンド
	m_BlendDescs[(int)BlendModeType::Alpha].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	m_BlendDescs[(int)BlendModeType::Alpha].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	m_BlendDescs[(int)BlendModeType::Alpha].RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

	//加算合成
	m_BlendDescs[(int)BlendModeType::Additive].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	m_BlendDescs[(int)BlendModeType::Additive].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	m_BlendDescs[(int)BlendModeType::Additive].RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

	//減算合成
	m_BlendDescs[(int)BlendModeType::Subtract].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	m_BlendDescs[(int)BlendModeType::Subtract].RenderTarget[0].BlendOp = D3D12_BLEND_OP_SUBTRACT;
	m_BlendDescs[(int)BlendModeType::Subtract].RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

	//乗算合成
	m_BlendDescs[(int)BlendModeType::Multiply].RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
	m_BlendDescs[(int)BlendModeType::Multiply].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	m_BlendDescs[(int)BlendModeType::Multiply].RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;

	//スクリーン合成
	m_BlendDescs[(int)BlendModeType::Screen].RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
	m_BlendDescs[(int)BlendModeType::Screen].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	m_BlendDescs[(int)BlendModeType::Screen].RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
}

BlendModeManager::~BlendModeManager () {

}

const D3D12_BLEND_DESC& BlendModeManager::GetBlendDesc (BlendModeType type) const {
	assert (type < BlendModeType::Count);
	return m_BlendDescs[(int)type];
}