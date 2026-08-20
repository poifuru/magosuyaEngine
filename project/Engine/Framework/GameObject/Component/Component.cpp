#include "PCH.h"
#include "Component.h"
#include "GameObject.h"

D3D12_GPU_VIRTUAL_ADDRESS Component::GetTransformAddress() const  {
	return gameObject_ ? gameObject_->GetTransformGPUAddress() : 0;
}