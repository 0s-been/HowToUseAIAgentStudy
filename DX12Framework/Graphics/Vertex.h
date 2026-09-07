// Vertex.h
// 정점 포맷과 입력 레이아웃.
//
// 입력 레이아웃의 SemanticName은 HLSL 정점 셰이더 입력 구조체의 시맨틱과
// 정확히 일치해야 한다. (Shaders/Basic_VS.hlsl 참고)

#pragma once
#include "../Core/stdafx.h"

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT2 uv;
};

// D3D12_APPEND_ALIGNED_ELEMENT를 쓰면 오프셋을 직접 계산하지 않아도 된다.
// 구조체 멤버 순서만 맞으면 자동으로 이어 붙는다.
inline const D3D12_INPUT_ELEMENT_DESC kVertexInputLayout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                                 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT,      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT,      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

constexpr UINT kVertexInputLayoutCount = _countof(kVertexInputLayout);
