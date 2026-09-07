// UploadBuffer.h
// CPU가 매 프레임 값을 바꿔 쓰는 업로드 힙 버퍼.
//
// 업로드 힙은 CPU에서 쓰기가 가능한 대신 GPU 읽기 성능이 낮다.
// 상수 버퍼처럼 매 프레임 바뀌는 작은 데이터에 적합하다.
// (정점/인덱스처럼 한 번 올리고 계속 읽는 데이터는 디폴트 힙을 써야 한다. Mesh 참고)
//
// Map을 해제하지 않고 계속 유지하는 것은 D3D12에서 권장되는 방식이다.
// Map/Unmap을 반복하면 오히려 비용이 든다.

#pragma once
#include "../Core/stdafx.h"
#include "../Core/DXException.h"
#include "../Core/Logger.h"
#include "D3D12Helpers.h"

template <typename T>
class UploadBuffer
{
public:
	UploadBuffer() = default;
	~UploadBuffer() { Shutdown(); }

	UploadBuffer(const UploadBuffer&) = delete;
	UploadBuffer& operator=(const UploadBuffer&) = delete;

	// isConstantBuffer가 true면 원소마다 256바이트로 정렬한다.
	// 상수 버퍼 뷰는 반드시 256바이트 경계에서 시작해야 한다는 D3D12 규칙 때문이다.
	bool Initialize(ID3D12Device* device, UINT elementCount, bool isConstantBuffer, const wchar_t* debugName)
	{
		try
		{
			m_elementByteSize = isConstantBuffer
				? DX::AlignConstantBufferSize(sizeof(T))
				: static_cast<UINT>(sizeof(T));
			m_elementCount = elementCount;

			const UINT64 totalSize = static_cast<UINT64>(m_elementByteSize) * elementCount;

			const D3D12_HEAP_PROPERTIES heapProps = DX::HeapProperties(D3D12_HEAP_TYPE_UPLOAD);
			const D3D12_RESOURCE_DESC desc = DX::BufferDesc(totalSize);

			ThrowIfFailed(device->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				// 업로드 힙 리소스는 이 상태로 만들어야 하며 이후 전이시킬 수 없다.
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_buffer)));

			if (debugName != nullptr)
			{
				m_buffer->SetName(debugName);
			}

			// 읽지 않을 것이므로 읽기 범위를 비워 둔다. (begin == end)
			const D3D12_RANGE readRange = { 0, 0 };
			ThrowIfFailed(m_buffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mapped)));
			return true;
		}
		catch (const DxException& e)
		{
			LOG_ERROR(L"업로드 버퍼 생성 실패: %s", e.ToString().c_str());
			return false;
		}
	}

	void Shutdown()
	{
		if (m_buffer != nullptr && m_mapped != nullptr)
		{
			m_buffer->Unmap(0, nullptr);
		}
		m_mapped = nullptr;
		m_buffer.Reset();
		m_elementByteSize = 0;
		m_elementCount = 0;
	}

	void CopyData(UINT index, const T& data)
	{
		if (m_mapped == nullptr || index >= m_elementCount)
		{
			return;
		}
		std::memcpy(m_mapped + static_cast<size_t>(index) * m_elementByteSize, &data, sizeof(T));
	}

	// 루트 상수 버퍼 뷰(SetGraphicsRootConstantBufferView)에 넘길 GPU 주소.
	D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress(UINT index) const
	{
		return m_buffer->GetGPUVirtualAddress() + static_cast<UINT64>(index) * m_elementByteSize;
	}

	ID3D12Resource* Get() const { return m_buffer.Get(); }
	UINT GetElementByteSize() const { return m_elementByteSize; }
	UINT GetElementCount() const { return m_elementCount; }

private:
	ComPtr<ID3D12Resource> m_buffer;
	BYTE* m_mapped = nullptr;
	UINT m_elementByteSize = 0;
	UINT m_elementCount = 0;
};
