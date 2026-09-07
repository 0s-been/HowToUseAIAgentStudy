// Camera.h
// 뷰/투영 행렬만 계산하는 단순 카메라.
// 왼손 좌표계(LH)를 쓴다. DirectXMath의 *LH 계열 함수와 짝을 맞춘다.

#pragma once
#include "../Core/stdafx.h"

class Camera
{
public:
	Camera();

	// fovYRadians: 세로 시야각, nearZ/farZ: 절두체 앞뒤 평면 거리
	void SetLens(float fovYRadians, float aspectRatio, float nearZ, float farZ);

	// 창 크기가 바뀌면 종횡비만 갱신한다.
	void SetAspectRatio(float aspectRatio);

	void LookAt(const DirectX::XMFLOAT3& position,
		const DirectX::XMFLOAT3& target,
		const DirectX::XMFLOAT3& up);

	DirectX::XMMATRIX GetView() const { return DirectX::XMLoadFloat4x4(&m_view); }
	DirectX::XMMATRIX GetProj() const { return DirectX::XMLoadFloat4x4(&m_proj); }
	DirectX::XMMATRIX GetViewProj() const { return DirectX::XMMatrixMultiply(GetView(), GetProj()); }

	const DirectX::XMFLOAT3& GetPosition() const { return m_position; }

private:
	void UpdateProjection();

	DirectX::XMFLOAT4X4 m_view;
	DirectX::XMFLOAT4X4 m_proj;
	DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };

	float m_fovY = DirectX::XM_PIDIV4;
	float m_aspectRatio = 16.0f / 9.0f;
	float m_nearZ = 0.1f;
	float m_farZ = 1000.0f;
};
