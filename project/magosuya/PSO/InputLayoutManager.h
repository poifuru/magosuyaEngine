#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <vector>
#include <unordered_map>

// 頂点レイアウトを識別するためのID
enum class InputLayoutType : uint32_t {
    Standard3D = 0, // POSITION, TEXCOORD, NORMALを持つレイアウト
    MeshShader = 1, // 将来のメッシュシェーダー用(InputLayout不要)
    
    Count           // 設定の個数
};

// InputLayoutの構造体（D3D12_INPUT_LAYOUT_DESCに相当するが、配列実体を保持）
struct InputLayoutData {
    std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
    D3D12_INPUT_LAYOUT_DESC desc; // D3D12_GRAPHICS_PIPELINE_STATE_DESCに渡す構造体
};

class InputLayoutManager {
public:     //メンバ関数
    static InputLayoutManager* GetInstance () {
        //初めて呼び出されたときに一回だけ初期化
        static InputLayoutManager instance;
        return &instance;
    }

    //登録したい設定を全部初期化
    void Initialize ();

    // D3D12_INPUT_LAYOUT_DESCへのポインタを返す
    const D3D12_INPUT_LAYOUT_DESC* GetInputLayout (InputLayoutType type) const;

private:    //メンバ変数
    // InputLayoutTypeと実体データのマップ
    std::unordered_map<InputLayoutType, InputLayoutData> m_LayoutCache;
};