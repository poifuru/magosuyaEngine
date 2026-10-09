#pragma once
#include "CameraOrganizer.h"

class BaseCamera {
public:
	virtual ~BaseCamera () = default;

	virtual void Initialize (const EulerTransform& transform) = 0;
	virtual void Update () = 0;
	virtual void ImGui () = 0;

	const CameraData& GetCameraData() const { return camera_; }
	const EulerTransform& GetEulerTransform () const { return camera_.transform; }
	
	Vector3 GetScale () const { return camera_.transform.scale; }
	void SetScale (const Vector3& scale) { camera_.transform.scale = scale; }
	Vector3 GetRotate () const { return camera_.transform.rotate; }
	void SetRotate (const Vector3& rotate) { camera_.transform.rotate = rotate; }
	Vector3 GetTranslate () const { return camera_.transform.translate; }
	void SetTranslate (const Vector3& translate) { camera_.transform.translate = translate; }
	
	const Matrix4x4& GetWorldMat () const { return camera_.world; }
	const Matrix4x4& GetViewMat () const { return camera_.view; }
	const Matrix4x4& GetProjMat () const { return camera_.proj; }
	const Matrix4x4& GetVPMat () const { return camera_.vp; }
	
	float GetNear() const { return camera_.nearClip; }
	float GetFar() const { return camera_.farClip; }

protected:
	CameraData camera_ = {};
};