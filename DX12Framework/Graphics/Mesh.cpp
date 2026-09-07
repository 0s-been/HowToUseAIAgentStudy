#include "Mesh.h"

#include "../Core/DXException.h"
#include "../Core/Logger.h"
#include "D3D12Helpers.h"

bool Mesh::CreateDefaultBuffer(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	const void* data,
	UINT64 byteSize,
	D3D12_RESOURCE_STATES finalState,
	ComPtr<ID3D12Resource>& outBuffer,
	ComPtr<ID3D12Resource>& outUploader,
	const wchar_t* debugName)
{
	try
	{
		// 1) GPU 전용 메모리에 최종 버퍼를 만든다. COMMON 상태로 시작한다.
		const D3D12_HEAP_PROPERTIES defaultHeap = DX::HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		const D3D12_RESOURCE_DESC bufferDesc = DX::BufferDesc(byteSize);

		ThrowIfFailed(device->CreateCommittedResource(
			&defaultHeap,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&outBuffer)));

		if (debugName != nullptr)
		{
			outBuffer->SetName(debugName);
		}

		// 2) CPU가 쓸 수 있는 임시 업로드 버퍼를 만든다.
		const D3D12_HEAP_PROPERTIES uploadHeap = DX::HeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		ThrowIfFailed(device->CreateCommittedResource(
			&uploadHeap,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&outUploader)));

		// 3) 업로드 버퍼에 데이터를 채운다.
		void* mapped = nullptr;
		const D3D12_RANGE readRange = { 0, 0 };	// 읽지 않는다.
		ThrowIfFailed(outUploader->Map(0, &readRange, &mapped));
		std::memcpy(mapped, data, static_cast<size_t>(byteSize));
		outUploader->Unmap(0, nullptr);

		// 4) 업로드 -> 디폴트 복사 명령을 기록한다.
		//    복사 대상은 COPY_DEST 상태여야 하고, 복사가 끝나면 실제 용도 상태로 되돌린다.
		const D3D12_RESOURCE_BARRIER toCopyDest = DX::TransitionBarrier(
			outBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &toCopyDest);

		commandList->CopyBufferRegion(outBuffer.Get(), 0, outUploader.Get(), 0, byteSize);

		const D3D12_RESOURCE_BARRIER toFinal = DX::TransitionBarrier(
			outBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, finalState);
		commandList->ResourceBarrier(1, &toFinal);

		return true;
	}
	catch (const DxException& e)
	{
		LOG_ERROR(L"디폴트 힙 버퍼 생성 실패: %s", e.ToString().c_str());
		return false;
	}
}

bool Mesh::Initialize(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	const std::vector<Vertex>& vertices,
	const std::vector<uint16_t>& indices,
	const wchar_t* debugName)
{
	if (vertices.empty() || indices.empty())
	{
		LOG_ERROR(L"메시 생성 실패: 정점 또는 인덱스가 비어 있다.");
		return false;
	}

	const UINT64 vertexByteSize = sizeof(Vertex) * vertices.size();
	const UINT64 indexByteSize = sizeof(uint16_t) * indices.size();

	if (!CreateDefaultBuffer(device, commandList, vertices.data(), vertexByteSize,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_vertexBuffer, m_vertexUploader, debugName))
	{
		return false;
	}

	if (!CreateDefaultBuffer(device, commandList, indices.data(), indexByteSize,
			D3D12_RESOURCE_STATE_INDEX_BUFFER, m_indexBuffer, m_indexUploader, debugName))
	{
		return false;
	}

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = static_cast<UINT>(vertexByteSize);

	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;	// uint16_t와 맞춰야 한다.
	m_indexBufferView.SizeInBytes = static_cast<UINT>(indexByteSize);

	m_indexCount = static_cast<UINT>(indices.size());

	LOG_INFO(L"메시 생성: %s (정점 %zu개, 인덱스 %zu개)",
		(debugName != nullptr) ? debugName : L"(이름 없음)", vertices.size(), indices.size());
	return true;
}

void Mesh::DisposeUploaders()
{
	m_vertexUploader.Reset();
	m_indexUploader.Reset();
}

void Mesh::Shutdown()
{
	DisposeUploaders();
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();
	m_vertexBufferView = {};
	m_indexBufferView = {};
	m_indexCount = 0;
}

void Mesh::Draw(ID3D12GraphicsCommandList* commandList) const
{
	if (!IsValid())
	{
		return;
	}

	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->IASetIndexBuffer(&m_indexBufferView);
	commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
}
