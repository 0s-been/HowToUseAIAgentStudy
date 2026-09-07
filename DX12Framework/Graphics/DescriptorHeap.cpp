#include "DescriptorHeap.h"

#include "../Core/DXException.h"
#include "../Core/Logger.h"

DescriptorHeap::~DescriptorHeap()
{
	Shutdown();
}

bool DescriptorHeap::Initialize(ID3D12Device* device,
	D3D12_DESCRIPTOR_HEAP_TYPE type,
	UINT capacity,
	bool shaderVisible,
	const wchar_t* debugName)
{
	try
	{
		// RTV/DSV 힙은 셰이더 가시로 만들 수 없다. 잘못 넘어오면 바로잡아 준다.
		if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
		{
			if (shaderVisible)
			{
				LOG_WARN(L"RTV/DSV 힙은 셰이더 가시로 만들 수 없다. shaderVisible을 false로 강제한다.");
			}
			shaderVisible = false;
		}

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = type;
		desc.NumDescriptors = capacity;
		desc.Flags = shaderVisible
			? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
			: D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;

		ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)));

		if (debugName != nullptr)
		{
			m_heap->SetName(debugName);
		}

		m_descriptorSize = device->GetDescriptorHandleIncrementSize(type);
		m_capacity = capacity;
		m_allocated = 0;
		m_shaderVisible = shaderVisible;

		m_cpuStart = m_heap->GetCPUDescriptorHandleForHeapStart();
		if (shaderVisible)
		{
			m_gpuStart = m_heap->GetGPUDescriptorHandleForHeapStart();
		}
		else
		{
			m_gpuStart.ptr = 0;
		}

		LOG_INFO(L"디스크립터 힙 생성: %s (칸 %u개, 칸 크기 %u바이트, 셰이더 가시 %s)",
			(debugName != nullptr) ? debugName : L"(이름 없음)",
			capacity, m_descriptorSize, shaderVisible ? L"예" : L"아니오");
		return true;
	}
	catch (const DxException& e)
	{
		LOG_ERROR(L"디스크립터 힙 생성 실패: %s", e.ToString().c_str());
		return false;
	}
}

void DescriptorHeap::Shutdown()
{
	m_heap.Reset();
	m_cpuStart = {};
	m_gpuStart = {};
	m_descriptorSize = 0;
	m_capacity = 0;
	m_allocated = 0;
	m_shaderVisible = false;
}

UINT DescriptorHeap::Allocate(UINT count)
{
	if (count == 0 || m_allocated + count > m_capacity)
	{
		LOG_ERROR(L"디스크립터 힙 공간 부족. 요청 %u칸, 남은 %u칸", count, m_capacity - m_allocated);
		return kInvalidIndex;
	}

	const UINT start = m_allocated;
	m_allocated += count;
	return start;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuHandle(UINT index) const
{
	// 힙의 시작 주소에서 (인덱스 x 칸 크기)만큼 떨어진 곳이 해당 칸이다.
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_cpuStart;
	handle.ptr += static_cast<SIZE_T>(index) * m_descriptorSize;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandle(UINT index) const
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_gpuStart;
	handle.ptr += static_cast<UINT64>(index) * m_descriptorSize;
	return handle;
}
