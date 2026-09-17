#include "PCH.h"
#include "CameraOrganizer.h"
#include "MainCameraComponent.h"
#include "VirtualCameraComponent.h"
#include "GameObject.h"
#include "MathFunction.h"
#include "Easing.h"

CameraOrganizer::~CameraOrganizer() {
}

void CameraOrganizer::Initialize() {
	mainCamera_ = nullptr;
	virtualCameras_.clear();

	currentVirtualCamera_ = nullptr;
	preVirtualCamera_ = nullptr;

	isBlending_ = false;
	blendTimer_ = 0.0f;
}

void CameraOrganizer::Update() {
	VirtualCameraComponent* targetCam = FindActiveVirtualCamera();
	if(!targetCam || !mainCamera_) return;	// ターゲットカメラ・メインカメラがnullなら早期リターン

	// アクティブな仮想カメラが切り替わった場合、ブレンドを開始する
	if(targetCam != currentVirtualCamera_) {
		if(currentVirtualCamera_) {
			preVirtualCamera_ = currentVirtualCamera_;
			isBlending_ = true;
			blendTimer_ = 0.0f;
		}
		currentVirtualCamera_ = targetCam;
	}

	Vector3 finalPos = {};
	Vector3 finalRot = {};
	currentFov_ = currentVirtualCamera_->GetFov();

	if (isBlending_ && preVirtualCamera_) {
		// 一旦ローカル変数に退避させておく
		VirtualCameraComponent* preCam = preVirtualCamera_;

		blendTimer_ += Time::GetDeltaTime(); // デルタタイム（仮で1/60固定）
		float t = blendTimer_ / blendDuration_;
		if (t >= 1.0f) {
			t = 1.0f;
			isBlending_ = false;
			preVirtualCamera_ = nullptr;
		}

		// イージング（SmoothStep）をかけて滑らかに補間する
		float easeT = Easing::easeInOutQuint(t);

		// 座標と回転、FOVを線形補間する
		finalPos = Math::Lerp(preCam->GetPosition(), currentVirtualCamera_->GetPosition(), easeT);
		finalRot = Math::Lerp(preCam->GetRotate(), currentVirtualCamera_->GetRotate(), easeT);
		currentFov_ = Math::Lerp(preCam->GetFov(), currentVirtualCamera_->GetFov(), easeT);
	} else {
		finalPos = currentVirtualCamera_->GetPosition();
		finalRot = currentVirtualCamera_->GetRotate();
	}

	// カメラシェイク処理
	if (shakeTimer_ > 0.0f) {
		shakeTimer_ -= Time::GetDeltaTime();
		if (shakeTimer_ < 0.0f) shakeTimer_ = 0.0f;

		if (shakeDuration_ > 0.0f) {
			float progress = shakeTimer_ / shakeDuration_;
			float currentIntensity = shakeIntensity_ * progress;

			float offsetX = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * currentIntensity;
			float offsetY = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * currentIntensity;
			float offsetZ = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * currentIntensity;

			finalPos.x += offsetX;
			finalPos.y += offsetY;
			finalPos.z += offsetZ;
		}
	}

	// 計算した最終的なカメラ姿勢を実体カメラオブジェクトに適用する
	auto& mainTrans = mainCamera_->GetGameObject()->GetTransform();
	mainTrans.translate = finalPos;
	mainTrans.rotate = finalRot;

	mainCamera_->UpdateMatrix();
}

void CameraOrganizer::Shake(float duration, float intensity) {
	shakeDuration_ = duration;
	shakeTimer_ = duration;
	shakeIntensity_ = intensity;
}

void CameraOrganizer::RegisterVirtualCamera(VirtualCameraComponent* virtualCamera) {
	if(std::find(virtualCameras_.begin(), virtualCameras_.end(), virtualCamera) == virtualCameras_.end()) {
		virtualCameras_.push_back(virtualCamera);
	}
}

void CameraOrganizer::UnregisterVirtualCamera(VirtualCameraComponent* virtualCamera) {
	auto it = std::find(virtualCameras_.begin(), virtualCameras_.end(), virtualCamera);
	if(it != virtualCameras_.end()) {
		virtualCameras_.erase(it);
	}
	if(currentVirtualCamera_ == virtualCamera) currentVirtualCamera_ = nullptr;
	if(preVirtualCamera_ == virtualCamera) preVirtualCamera_ = nullptr;
}

void CameraOrganizer::UnregisterMainCamera(MainCameraComponent* mainCamera) {
	if (mainCamera_ == mainCamera) {
		mainCamera_ = nullptr;
	}
}

CameraData& CameraOrganizer::GetCameraData() {
	// メインカメラの実体から最終行列を取得する
	if (mainCamera_) {
		// メインカメラがまだ登録されていない場合は、ダミーデータを返す
		return mainCamera_->GetCameraData();
	}
	
	// メインカメラがいない場合の静的なダミーデータ
	static CameraData defaultCameraData;
	static bool isInitialized = false;
	if (!isInitialized) {
		defaultCameraData = {};
		defaultCameraData.transform.scale = { 1.0f, 1.0f, 1.0f };
		defaultCameraData.transform.rotate = { 0.0f, 0.0f, 0.0f };
		defaultCameraData.transform.translate = { 0.0f, 0.0f, -10.0f };
		defaultCameraData.world = Math::MakeIdentity4x4();
		defaultCameraData.view = Math::MakeIdentity4x4();
		defaultCameraData.proj = Math::MakeIdentity4x4();
		defaultCameraData.vp = Math::MakeIdentity4x4();
		isInitialized = true;
	}
	return defaultCameraData;
}

VirtualCameraComponent* CameraOrganizer::FindActiveVirtualCamera() {
	// 仮想カメラのコンテナが空なら早期リターン
	if(virtualCameras_.empty()) return nullptr;

	// 優先度(Priority)が一番高いものを探す
	VirtualCameraComponent* activeCam = virtualCameras_[0];
	for(auto* cam : virtualCameras_) {
		if(cam->GetPriority() > activeCam->GetPriority()) {
			activeCam = cam;
		}
	}

	return activeCam;
}
