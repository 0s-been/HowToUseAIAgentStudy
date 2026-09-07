#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
	XMStoreFloat4x4(&m_view, XMMatrixIdentity());
	XMStoreFloat4x4(&m_proj, XMMatrixIdentity());
	UpdateProjection();
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

void Camera::LookAt(const XMFLOAT3& position, const XMFLOAT3& target, const XMFLOAT3& up)
{
	m_position = position;

	const XMVECTOR eye = XMLoadFloat3(&position);
	const XMVECTOR at = XMLoadFloat3(&target);
	const XMVECTOR upVec = XMLoadFloat3(&up);

	XMStoreFloat4x4(&m_view, XMMatrixLookAtLH(eye, at, upVec));
}
