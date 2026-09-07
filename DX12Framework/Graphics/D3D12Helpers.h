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
}
