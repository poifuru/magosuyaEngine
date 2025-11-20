#include "InputLayoutManager.h"
#include <cassert>

InputLayoutManager::InputLayoutManager () {
	InputLayoutData data;

	//InputLayoutの設定
	//POSITION
	data.elements.push_back ({
		"POSITION",
		0,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
							 });
	//TEXCODE
	data.elements.push_back ({
		"TEXCOORD",
		0,
		DXGI_FORMAT_R32G32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
							 });

	data.elements.push_back ({
		"NORMAL",
		0,
		DXGI_FORMAT_R32G32B32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
							 });

	// --- 2. D3D12_INPUT_LAYOUT_DESCの作成 ---
	data.desc.pInputElementDescs = data.elements.data ();
	data.desc.NumElements = (UINT)data.elements.size ();

	// --- 3. キャッシュに登録 ---
	m_LayoutCache[InputLayoutType::Standard3D] = std::move (data);
}

InputLayoutManager::~InputLayoutManager () {

}

const D3D12_INPUT_LAYOUT_DESC* InputLayoutManager::GetInputLayout (InputLayoutType type) const {
	if (m_LayoutCache.count (type)) {
		return &m_LayoutCache.at (type).desc;
	}
	assert (false && "InputLayoutType not found in cache!");
	return nullptr;
}