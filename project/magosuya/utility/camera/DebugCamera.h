#pragma once
#include <Windows.h>
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <d3d12.h>
#include <d3d12sdklayers.h>
#pragma comment(lib, "d3d12.lib")
#include <dxgi1_6.h>
#pragma comment(lib, "dxgi.lib")
#pragma	comment(lib, "dxguid.lib")
#include "struct.h"
#include "MathFunction.h"
#include "InputManager.h"

class DebugCamera{
public:	//メンバ関数
	//コンストラクタ
	DebugCamera ();

	//初期化
	void Initialize ();

	//更新
	void Updata (HWND hwnd, HRESULT hr, InputManager* inputManager);

	//ゲッター
	Matrix4x4 GetWorldMatrix () { return worldMatrix_; }
	Matrix4x4 GetViewMatrix () { return viewMatrix_; }
	Matrix4x4 GetProjectionMatrix () { return projectionMatrix_; }
	bool GetTatchImGui () { return tatchImGui_; }
	void SetTatchImGui (bool flag) { tatchImGui_ = flag; }

private:	//メンバ変数
	Transform transform_;

	//ワールド座標
	Matrix4x4 worldMatrix_;
	
	//ビュー行列
	Matrix4x4 viewMatrix_;
	
	//射影行列
	Matrix4x4 projectionMatrix_;

	//=======カメラの挙動に使う変数=======//
	//カメラの正面に対しての前後左右に移動
	Vector3 forward_;	//カメラの前方ベクトル
	Vector3 right_;		//カメラの右ベクトル
	float speed_;		//移動スピード
	Vector3 move_;		//実際に移動に反映するベクトル

	//ドラッグで視点を変える
	float sensitivity_;	//ドラッグの感度
	float pitchOver_;	//上を向きすぎるのを防止する

	bool tatchImGui_ = false;	//ImGuiにカーソルが触れているか
};

