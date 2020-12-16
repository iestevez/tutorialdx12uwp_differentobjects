#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <random>

using namespace DirectX;
namespace Geo {

	XMMATRIX GetRandomPointInsideFrustum(const XMMATRIX  &projection, const XMMATRIX &view, const float r, const float minDistance, const float  maxDistance) {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<> disx(-r, r);
		std::uniform_real_distribution<> disy(-1.0, 1.0);
		std::uniform_real_distribution<> disz(minDistance, maxDistance);

		
		XMMATRIX invView = XMMatrixInverse(nullptr, view);
		XMMATRIX invProjection = XMMatrixInverse(nullptr, projection);
		float x = static_cast<float>(disx(gen));
		float y = static_cast<float>(disy(gen));
		float z = static_cast<float>(disz(gen));
		//x = 0.0f;
		//y = 10.0f;
		//z = 1.0f;
		XMFLOAT4X4 mat;
		XMStoreFloat4x4(&mat,projection);
		float A = mat._33;
		float B = mat._43;


		XMVECTOR vndc_zcorrected = { x*z,y*z,A*z+B,z}; //(NDCinv)
		XMVECTOR vworld = XMVector4Transform(vndc_zcorrected, invProjection);
		vworld = XMVector4Transform(vworld, invView);
		return XMMatrixTranslation(XMVectorGetX(vworld), XMVectorGetY(vworld), XMVectorGetZ(vworld));
		
	}

	XMMATRIX GetRandomRotationMatrix() {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<> disyawroll(0.0, XM_2PI);
		std::uniform_real_distribution<> dispitch(-XM_PIDIV2, XM_PIDIV2);
		float pitch = static_cast<float>(dispitch(gen));
		float yaw = static_cast<float>(disyawroll(gen));
		float roll = static_cast<float>(disyawroll(gen));
		return XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
	}


}