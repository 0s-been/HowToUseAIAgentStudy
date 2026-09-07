#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
	XMStoreFloat4x4(&m_view, XMMatrixIdentity());
	XMStoreFloat4x4(&m_proj, XMMatrixIdentity());

	UpdateProjection();
	RebuildBasis();
	UpdateViewMatrix();
}

void Camera::SetLens(float fovYRadians, float aspectRatio, float nearZ, float farZ)
{
	m_fovY = fovYRadians;
	m_aspectRatio = aspectRatio;
	m_nearZ = nearZ;
	m_farZ = farZ;
	UpdateProjection();
}

void Camera::SetAspectRatio(float aspectRatio)
{
	if (aspectRatio <= 0.0f)
	{
		return;
	}
	m_aspectRatio = aspectRatio;
	UpdateProjection();
}

void Camera::UpdateProjection()
{
	XMStoreFloat4x4(&m_proj, XMMatrixPerspectiveFovLH(m_fovY, m_aspectRatio, m_nearZ, m_farZ));
}

void Camera::SetPosition(const XMFLOAT3& position)
{
	m_position = position;
	m_viewDirty = true;
}

void Camera::LookAt(const XMFLOAT3& position, const XMFLOAT3& target)
{
	m_position = position;

	const XMVECTOR direction = XMVector3Normalize(
		XMVectorSubtract(XMLoadFloat3(&target), XMLoadFloat3(&position)));

	XMFLOAT3 dir;
	XMStoreFloat3(&dir, direction);

	// RebuildBasis가 만드는 방향 벡터를 거꾸로 푼 것이다.
	//   look = ( cos(pitch)*sin(yaw), -sin(pitch), cos(pitch)*cos(yaw) )
	// 이므로 y 성분에서 pitch가, x와 z의 비에서 yaw가 나온다.
	m_pitch = -asinf(std::clamp(dir.y, -1.0f, 1.0f));
	m_yaw = atan2f(dir.x, dir.z);

	m_pitch = std::clamp(m_pitch, -kMaxPitch, kMaxPitch);

	RebuildBasis();
	m_viewDirty = true;
}

void Camera::Walk(float distance)
{
	// position += look * distance
	const XMVECTOR position = XMVectorMultiplyAdd(
		XMVectorReplicate(distance), XMLoadFloat3(&m_look), XMLoadFloat3(&m_position));
	XMStoreFloat3(&m_position, position);
	m_viewDirty = true;
}

void Camera::Strafe(float distance)
{
	const XMVECTOR position = XMVectorMultiplyAdd(
		XMVectorReplicate(distance), XMLoadFloat3(&m_right), XMLoadFloat3(&m_position));
	XMStoreFloat3(&m_position, position);
	m_viewDirty = true;
}

void Camera::Fly(float distance)
{
	// 카메라의 up이 아니라 '월드' 위쪽으로 움직인다.
	// 위를 올려다보는 중에도 상승 방향이 흔들리지 않아 조작이 예측 가능해진다.
	m_position.y += distance;
	m_viewDirty = true;
}

void Camera::AddYaw(float radians)
{
	m_yaw += radians;
	// 값이 무한정 커지면서 정밀도가 떨어지는 것을 막는다.
	m_yaw = XMScalarModAngle(m_yaw);

	RebuildBasis();
	m_viewDirty = true;
}

void Camera::AddPitch(float radians)
{
	// 위아래로 뒤집히지 않도록 여기서 잘라낸다.
	m_pitch = std::clamp(m_pitch + radians, -kMaxPitch, kMaxPitch);

	RebuildBasis();
	m_viewDirty = true;
}

void Camera::RebuildBasis()
{
	// pitch(X축) -> yaw(Y축) 순으로 회전한 기준 방향 (0, 0, 1)이 곧 look이다.
	const XMMATRIX rotation = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
	const XMVECTOR look = XMVector3Normalize(
		XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation));

	// 월드 up과 look의 외적이 right가 된다. (왼손 좌표계 기준)
	// roll을 만들지 않으려면 카메라의 up이 아니라 '월드' up을 써야 한다.
	const XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	const XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, look));
	const XMVECTOR up = XMVector3Cross(look, right);

	XMStoreFloat3(&m_look, look);
	XMStoreFloat3(&m_right, right);
	XMStoreFloat3(&m_up, up);
}

void Camera::UpdateViewMatrix()
{
	if (!m_viewDirty)
	{
		return;
	}

	// LookAtLH는 '바라볼 지점'을, LookToLH는 '바라볼 방향'을 받는다.
	// 여기서는 방향 벡터를 이미 들고 있으므로 LookToLH가 맞다.
	XMStoreFloat4x4(&m_view, XMMatrixLookToLH(
		XMLoadFloat3(&m_position),
		XMLoadFloat3(&m_look),
		XMLoadFloat3(&m_up)));

	m_viewDirty = false;
}
