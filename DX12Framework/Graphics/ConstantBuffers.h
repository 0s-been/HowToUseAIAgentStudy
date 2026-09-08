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

	// 체커 무늬(바닥 등)를 정점 색이 아니라 픽셀 셰이더에서 절차적으로 계산하기 위한 값.
	// checkerCellSize가 0이면 체커를 쓰지 않고 baseColor(정점색과 곱해진 값)만 그대로 쓴다.
	// 0보다 크면 baseColor를 첫 번째 체커 색으로 두고 checkerColorB와 섞는다.
	//
	// 왜 정점에 색을 굽지 않는가: 체커 경계가 화면에서 1픽셀보다 작아지는 거리(바닥 먼 쪽)에서는
	// MSAA(정해진 샘플 개수)로도 무늬가 다 잡히지 않아 카메라가 움직일 때 경계가 어른거린다.
	// 픽셀 셰이더에서 화면 공간 도함수(ddx/ddy)로 폭을 알고 그만큼 미리 흐려 주면,
	// 무늬가 멀어질수록 자연스럽게 중간 회색으로 수렴해 그 현상이 없어진다.
	DirectX::XMFLOAT4 checkerColorB;
	float checkerCellSize;
	DirectX::XMFLOAT3 checkerPadding;
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
