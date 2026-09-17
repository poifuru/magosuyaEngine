#pragma once

// 前方宣言
class MainCameraComponent;
class VirtualCameraComponent;

struct CameraData {
	EulerTransform transform;
	Matrix4x4 world;
	Matrix4x4 view;
	Matrix4x4 proj;
	Matrix4x4 vp;
	float nearClip = 0.1f;
	float farClip = 1000.f;
};

class CameraOrganizer {
public:
	static CameraOrganizer* GetInstance () {
		//初めて呼び出されたときに一回だけ初期化
		static CameraOrganizer instance;
		return &instance;
	}
	~CameraOrganizer ();

	//初期化関数
	void Initialize ();

	//アクティブカメラの更新処理
	void Update ();

	// 現在アクティブな仮想カメラを取得
	VirtualCameraComponent* GetCurrentVirtualCamera() const { return currentVirtualCamera_; }

	// メインカメラの登録
	void RegisterMainCamera(MainCameraComponent* mainCamera) { mainCamera_ = mainCamera; }

	// 仮想カメラの登録
	void RegisterVirtualCamera(VirtualCameraComponent* virtualCamera);

	// 仮想カメラの登録解除
	void UnregisterVirtualCamera(VirtualCameraComponent* virtualCamera);

	// メインカメラの登録解除
	void UnregisterMainCamera(MainCameraComponent* mainCamera);

	// 外部から描画情報をもらうためのインターフェース
	CameraData& GetCameraData();
	float GetActiveFov() const { return currentFov_; }

	// カメラシェイクのリクエスト
	void Shake(float duration = 0.35f, float intensity = 0.4f);

private:
	//コンストラクタを禁止
	CameraOrganizer () = default;
	// コピーコンストラクタと代入演算子を禁止
	CameraOrganizer (const CameraOrganizer&) = delete;
	CameraOrganizer& operator=(const CameraOrganizer&) = delete;
	CameraOrganizer (CameraOrganizer&&) = delete;
	CameraOrganizer& operator=(CameraOrganizer&&) = delete;

private:
	// 最優先の仮想カメラを決定する
	VirtualCameraComponent* FindActiveVirtualCamera();

private:
	MainCameraComponent* mainCamera_ = nullptr;			// メインカメラ
	std::vector<VirtualCameraComponent*> virtualCameras_;	// 仮想カメラのコンテナ

	VirtualCameraComponent* currentVirtualCamera_ = nullptr;	// 現在の仮想カメラ
	VirtualCameraComponent* preVirtualCamera_ = nullptr;	// ひとつ前の仮想カメラ

	// ブレンド（補間）用の変数
	float blendTimer_ = 0.0f;
	float blendDuration_ = 1.5f; // 切り替えかける時間
	bool isBlending_ = false;
	float currentFov_ = 0.45f;

	// カメラシェイク用の変数
	float shakeTimer_ = 0.0f;
	float shakeDuration_ = 0.0f;
	float shakeIntensity_ = 0.0f;
};