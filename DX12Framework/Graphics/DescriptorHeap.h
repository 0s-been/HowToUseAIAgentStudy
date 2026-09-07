// DescriptorHeap.h
// ID3D12DescriptorHeap 래퍼 + 선형 할당자.
//
// D3D11의 View(RTV/SRV/...)는 각각이 독립된 COM 객체였지만,
// D3D12에서는 "힙"이라는 커다란 배열을 먼저 만들고 그 안의 칸(인덱스)에 뷰를 기록한다.
// 칸 하나의 크기는 하드웨어마다 다르므로 GetDescriptorHandleIncrementSize로 조회해야 한다.
//
// 이 클래스는 해제를 지원하지 않는 단순 선형 할당자다.
// 수명이 프로그램 전체와 같은 RTV/DSV에는 이 정도로 충분하다.
// (텍스처를 붙일 때는 shaderVisible=true인 CBV_SRV_UAV 힙으로 그대로 재사용하면 된다.)

#pragma once
#include "../Core/stdafx.h"

class DescriptorHeap
{
public:
	static constexpr UINT kInvalidIndex = UINT_MAX;

	DescriptorHeap() = default;
	~DescriptorHeap();

	DescriptorHeap(const DescriptorHeap&) = delete;
	DescriptorHeap& operator=(const DescriptorHeap&) = delete;

	// shaderVisible은 셰이더가 직접 인덱싱하는 힙(CBV_SRV_UAV, SAMPLER)에만 true로 준다.
	// RTV/DSV 힙은 셰이더 가시 힙으로 만들 수 없다.
	bool Initialize(ID3D12Device* device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		UINT capacity,
		bool shaderVisible,
		const wchar_t* debugName);
	void Shutdown();

	// 연속된 count칸을 할당하고 시작 인덱스를 반환한다. 공간이 없으면 kInvalidIndex.
	UINT Allocate(UINT count = 1);

	// 할당 위치를 처음으로 되돌린다. (프레임마다 재사용하는 힙에 쓴다)
	void Reset() { m_allocated = 0; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT index) const;

	ID3D12DescriptorHeap* Get() const { return m_heap.Get(); }
	UINT GetCapacity() const { return m_capacity; }
	UINT GetAllocatedCount() const { return m_allocated; }
	UINT GetDescriptorSize() const { return m_descriptorSize; }
	bool IsShaderVisible() const { return m_shaderVisible; }

private:
	ComPtr<ID3D12DescriptorHeap> m_heap;

	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuStart = {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuStart = {};

	UINT m_descriptorSize = 0;
	UINT m_capacity = 0;
	UINT m_allocated = 0;
	bool m_shaderVisible = false;
};
