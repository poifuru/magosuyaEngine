#include "InputLayoutManager.h"
#include <cassert>

void InputLayoutManager::Initialize () {
	InputLayoutData data;

	//InputLayoutの設定
	//***Standard3D***//
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

	//Descの作成
	data.desc.pInputElementDescs = data.elements.data ();
	data.desc.NumElements = (UINT)data.elements.size ();

	//キャッシュに登録
	m_LayoutCache[InputLayoutType::Standard3D] = std::move (data);
	//新しい設定を登録するためにdataをclear
	data.elements.clear ();
	//******//

	//***Particle***//
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
		"COLOR",
		0,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
							 });

	//Descの作成
	data.desc.pInputElementDescs = data.elements.data ();
	data.desc.NumElements = (UINT)data.elements.size ();

	//キャッシュに登録
	m_LayoutCache[InputLayoutType::Particle] = std::move (data);
	//新しい設定を登録するためにdataをclear
	data.elements.clear ();
	//******//
}

const D3D12_INPUT_LAYOUT_DESC* InputLayoutManager::GetInputLayout (InputLayoutType type) const {
	if (m_LayoutCache.count (type)) {
		return &m_LayoutCache.at (type).desc;
	}
	assert (false && "InputLayoutType not found in cache!");
	return nullptr;
}