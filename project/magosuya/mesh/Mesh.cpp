#include "Mesh.h"
#include "MathFunction.h"
#include "LineRenderer.h"
#include "MaxMeshNum.h"

void Mesh::DrawLine (LineVertexData* data, const Matrix4x4& vp) {
	//rendererのインスタンスを取得
	LineRenderer* renderer_ = LineRenderer::GetInstance ();

	//頂点データを送信
	renderer_->UpdateVertexData (data);

	//実際にlineForGPUに中身を入れる
	LineForGPU lineForGPU[MaxMeshNum::Line];
	//ワールド座標作成用のローカル変数を宣言
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	Vector3 rotate = { 0.0f, 0.0f, 0.0f };
	for (uint32_t i = 0; i < VertexNum::Line * MaxMeshNum::Line; ++i) {
		lineForGPU[i].World = MakeAffineMatrix (scale, rotate, data[i].position);
		lineForGPU[i].WVP = Multiply (lineForGPU[i].World, vp);
	}
	//作ったデータを送信
	renderer_->TransferData (lineForGPU);

	renderer_->Draw ();
}
