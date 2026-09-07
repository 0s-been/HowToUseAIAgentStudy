// Camera.h
// 1인칭 방식으로 움직이는 카메라.
// 왼손 좌표계(LH)를 쓴다. DirectXMath의 *LH 계열 함수와 짝을 맞춘다.
//
// [회전을 yaw/pitch 각도로 들고 있는 이유]
// 기저 벡터(right/up/look)를 매번 조금씩 회전시키는 방식도 있지만,
// 부동소수점 오차가 쌓이면서 서서히 기울어지는(roll) 문제가 생긴다.
// 각도를 누적해 두고 매번 처음부터 벡터를 새로 만들면 그런 누적 오차가 없고,
// 위아래 시야 제한(pitch 클램프)도 자연스럽게 걸 수 있다.

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

	// 위치와 바라볼 지점으로 카메라를 맞춘다. yaw/pitch가 여기에 맞춰 역산된다.
	void LookAt(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target);

	void SetPosition(const DirectX::XMFLOAT3& position);

	// --- 이동 (distance는 부호 있는 거리) ---
	void Walk(float distance);		// 바라보는 방향으로 전진/후진
	void Strafe(float distance);	// 오른쪽/왼쪽으로 평행 이동
	void Fly(float distance);		// 월드 위쪽으로 상승/하강

	// --- 회전 (라디안) ---
	void AddYaw(float radians);		// 월드 Y축 기준 좌우 회전
	void AddPitch(float radians);	// 위아래 회전. 수직 근처에서 뒤집히지 않도록 제한된다

	// 이동/회전 후 한 번 호출한다. 실제로 바뀐 것이 없으면 아무 일도 하지 않는다.
	void UpdateViewMatrix();

	DirectX::XMMATRIX GetView() const { return DirectX::XMLoadFloat4x4(&m_view); }
	DirectX::XMMATRIX GetProj() const { return DirectX::XMLoadFloat4x4(&m_proj); }
	DirectX::XMMATRIX GetViewProj() const { return DirectX::XMMatrixMultiply(GetView(), GetProj()); }

	const DirectX::XMFLOAT3& GetPosition() const { return m_position; }
	const DirectX::XMFLOAT3& GetLook() const { return m_look; }
	const DirectX::XMFLOAT3& GetRight() const { return m_right; }

	float GetYaw() const { return m_yaw; }
	float GetPitch() const { return m_pitch; }

private:
	void UpdateProjection();
	// yaw/pitch로부터 look/right/up 기저 벡터를 새로 만든다.
	void RebuildBasis();

	// 위아래로 완전히 수직이 되면 기저 벡터 계산이 무너진다.
	// 89도에서 멈춰 세워 그 지점에 닿지 않게 한다.
	static constexpr float kMaxPitch = DirectX::XM_PI * 89.0f / 180.0f;

	DirectX::XMFLOAT4X4 m_view;
	DirectX::XMFLOAT4X4 m_proj;

	DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_right = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_up = { 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 m_look = { 0.0f, 0.0f, 1.0f };

	float m_yaw = 0.0f;		// 월드 Y축 기준 각도
	float m_pitch = 0.0f;	// 양수면 아래를 본다

	float m_fovY = DirectX::XM_PIDIV4;
	float m_aspectRatio = 16.0f / 9.0f;
	float m_nearZ = 0.1f;
	float m_farZ = 1000.0f;

	// 뷰 행렬을 다시 만들어야 하는지. 매 프레임 무조건 계산하는 낭비를 막는다.
	bool m_viewDirty = true;
};
