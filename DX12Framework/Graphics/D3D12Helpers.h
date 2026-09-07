// D3D12Helpers.h
// D3D12 구조체를 채우는 짧은 헬퍼 모음.
//
// 흔히 쓰이는 d3dx12.h는 Windows SDK에 포함되어 있지 않고 별도로 받아야 한다.
// 이 프레임워크는 외부 의존성을 두지 않기 위해 필요한 것만 직접 만들어 쓴다.

#pragma once
#include "../Core/stdafx.h"

namespace DX
{
	// 상수 버퍼는 256바이트 경계로 정렬되어야 한다는 D3D12 규칙이 있다.
	// 예) 실제 크기가 100바이트여도 256바이트를 잡아야 한다.
	constexpr UINT AlignUp(UINT value, UINT alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	constexpr UINT AlignConstantBufferSize(UINT byteSize)
	{
		return AlignUp(byteSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	inline D3D12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE type)
	{
		D3D12_HEAP_PROPERTIES props = {};
		props.Type = type;
		props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		props.CreationNodeMask = 1;
		props.VisibleNodeMask = 1;
		return props;
	}

	// 정점/인덱스/상수 버퍼처럼 구조가 없는 선형 버퍼용 설명자.
	inline D3D12_RESOURCE_DESC BufferDesc(UINT64 byteSize, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = byteSize;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		// 버퍼는 반드시 ROW_MAJOR여야 한다.
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = flags;
		return desc;
	}

	inline D3D12_RESOURCE_DESC Texture2DDesc(DXGI_FORMAT format, UINT64 width, UINT height,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, UINT16 mipLevels = 1)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment = 0;
		desc.Width = width;
		desc.Height = height;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = mipLevels;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		// 텍스처는 드라이버가 최적 배치를 고르도록 UNKNOWN으로 둔다.
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = flags;
		return desc;
	}

	// 리소스 상태 전이 배리어.
	// D3D12는 D3D11과 달리 "이 리소스를 지금 어떤 용도로 쓰는지"를 직접 알려줘야 한다.
	// 예) 백버퍼: PRESENT -> RENDER_TARGET -> PRESENT
	inline D3D12_RESOURCE_BARRIER TransitionBarrier(ID3D12Resource* resource,
		D3D12_RESOURCE_STATES before,
		D3D12_RESOURCE_STATES after,
		UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = before;
		barrier.Transition.StateAfter = after;
		barrier.Transition.Subresource = subresource;
		return barrier;
	}

	// ---- 파이프라인 상태 기본값 ----
	// d3dx12.h의 CD3DX12_*_DESC(D3D12_DEFAULT)에 해당하는 값들을 직접 채운다.

	inline D3D12_RASTERIZER_DESC DefaultRasterizerDesc()
	{
		D3D12_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D12_FILL_MODE_SOLID;
		desc.CullMode = D3D12_CULL_MODE_BACK;
		// 정점을 시계 방향으로 감았을 때 앞면으로 본다. (왼손 좌표계 기준)
		desc.FrontCounterClockwise = FALSE;
		desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		desc.DepthClipEnable = TRUE;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;
		desc.ForcedSampleCount = 0;
		desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		return desc;
	}

	// 블렌딩 없는 불투명 렌더 타겟.
	inline D3D12_BLEND_DESC OpaqueBlendDesc()
	{
		D3D12_BLEND_DESC desc = {};
		desc.AlphaToCoverageEnable = FALSE;
		desc.IndependentBlendEnable = FALSE;

		D3D12_RENDER_TARGET_BLEND_DESC rt = {};
		rt.BlendEnable = FALSE;
		rt.LogicOpEnable = FALSE;
		rt.SrcBlend = D3D12_BLEND_ONE;
		rt.DestBlend = D3D12_BLEND_ZERO;
		rt.BlendOp = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_ZERO;
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		rt.LogicOp = D3D12_LOGIC_OP_NOOP;
		rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			desc.RenderTarget[i] = rt;
		}
		return desc;
	}

	// 깊이 테스트 켜짐, 스텐실 꺼짐.
	inline D3D12_DEPTH_STENCIL_DESC DefaultDepthStencilDesc()
	{
		D3D12_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		desc.StencilEnable = FALSE;
		desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

		const D3D12_DEPTH_STENCILOP_DESC stencilOp =
		{
			D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
			D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS
		};
		desc.FrontFace = stencilOp;
		desc.BackFace = stencilOp;
		return desc;
	}
}
