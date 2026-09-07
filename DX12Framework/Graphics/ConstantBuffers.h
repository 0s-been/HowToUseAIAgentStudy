// ConstantBuffers.h
// CPU와 HLSL이 공유하는 상수 버퍼 구조체.
// Shaders/Basic_VS.hlsl, Basic_PS.hlsl의 cbuffer와 필드 순서/크기가 정확히 일치해야 한다.
//
// [행렬 규약]
// DirectXMath의 XMMATRIX는 행 우선(row-major)으로 저장되고,
// HLSL은 상수 버퍼의 행렬을 기본적으로 열 우선(column-major)으로 해석한다.
// 그래서 CPU에서 XMMatrixTranspose로 한 번 뒤집어 올리면 두 규약이 서로 상쇄되어
// 셰이더에서는 원래 행렬이 그대로 보인다.
// 이때 곱셈 순서는 행벡터 규약을 따른다: mul(float4(pos, 1.0f), gWorld)

#pragma once
#include "../Core/stdafx.h"

// 오브젝트마다 달라지는 값 (b0)
struct ObjectConstants
{
	DirectX::XMFLOAT4X4 world;
	// 비균등 스케일에서도 법선이 올바르게 변환되도록 역전치 행렬을 따로 넘긴다.
	DirectX::XMFLOAT4X4 worldInvTranspose;
	DirectX::XMFLOAT4 baseColor;
};

// 프레임(패스) 전체에서 공유하는 값 (b1)
struct PassConstants
{
	DirectX::XMFLOAT4X4 viewProj;

	DirectX::XMFLOAT3 eyePosW;
	float totalTime;

	// 빛이 나아가는 방향 (정규화되어 있어야 한다)
	DirectX::XMFLOAT3 lightDirection;
	float padding;

	DirectX::XMFLOAT4 lightColor;
	DirectX::XMFLOAT4 ambientColor;
};
