#include "Mesh.h"
#include "MathFunction.h"
#include "LineRenderer.h"
#include "CubeRenderer.h"
#include "MaxMeshNum.h"

void Mesh::DrawLine (LineVertexData* data, const Matrix4x4& vp) {
	//=========================//
	//ここでやるのはデータの転送だけ//
	//=========================//

	//rendererのインスタンスを取得
	LineRenderer* renderer_ = LineRenderer::GetInstance ();

	//頂点データを送信
	renderer_->UpdateVertexData (data);

	//実際にlineForGPUに中身を入れる
	LineForGPU lineForGPU;
	//ワールド座標作成用のローカル変数を宣言
	Vector3 center = (data[0].position + data[1].position) / 2.0f;
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	Vector3 rotate = { 0.0f, 0.0f, 0.0f };
	lineForGPU.World = MakeAffineMatrix (scale, rotate, center);
	lineForGPU.WVP = Multiply (lineForGPU.World, vp);
	//作ったデータを送信(1インスタンス分)
	renderer_->TransferData (lineForGPU);
}

void Mesh::DrawCube (CubeData* data, const Matrix4x4& vp) {
	//=========================//
	//ここでやるのはデータの転送だけ//
	//=========================//

	//rendererのインスタンスを取得
	CubeRenderer* renderer_ = CubeRenderer::GetInstance ();

	//頂点データを送信
	renderer_->UpdateVertexData (data);

	//実際にlineForGPUに中身を入れる
	CubeForGPU cubeForGPU;
	//ワールド座標作成用のローカル変数を宣言
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	Vector3 rotate = { 0.0f, 0.0f, 0.0f };
	cubeForGPU.World = MakeAffineMatrix (scale, rotate, data->center);
	cubeForGPU.WVP = Multiply (cubeForGPU.World, vp);
	//作ったデータを送信(1インスタンス分)
	renderer_->TransferData (cubeForGPU);
}

void Mesh::AllDrawing () {
	//最後にまとめて描画処理を流し込む
	LineRenderer::GetInstance ()->Draw ();
	CubeRenderer::GetInstance ()->Draw ();
}
