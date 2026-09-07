// Mesh.h
// 정점/인덱스 버퍼를 디폴트 힙에 올려 두고 그리는 클래스.
//
// [왜 업로드 힙이 아니라 디폴트 힙인가]
// 업로드 힙은 CPU가 쓸 수 있지만 GPU 읽기 성능이 낮다.
// 한 번 올리고 계속 읽기만 하는 정점/인덱스 데이터는 디폴트 힙(GPU 전용 메모리)에 두어야 한다.
// 다만 CPU가 디폴트 힙에 직접 쓸 수 없으므로, 임시 업로드 버퍼를 거쳐 GPU가 복사하게 한다.
//
// 사용 순서가 중요하다:
//   1) Initialize(...)          <- 복사 명령이 커맨드 리스트에 '기록'만 된다
//   2) 커맨드 리스트 Close/Execute
//   3) 큐 Flush (복사가 실제로 끝날 때까지 대기)
//   4) DisposeUploaders()       <- 그제서야 임시 버퍼를 버릴 수 있다

#pragma once
#include "../Core/stdafx.h"
#include "Vertex.h"

class Mesh
{
public:
	Mesh() = default;
	~Mesh() = default;

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	// commandList에 복사 명령을 기록한다. 호출 시점에 복사가 끝나는 것이 아니다.
	bool Initialize(ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		const std::vector<Vertex>& vertices,
		const std::vector<uint16_t>& indices,
		const wchar_t* debugName);

	void Shutdown();

	// GPU 복사가 끝난 뒤 호출해 임시 업로드 버퍼를 해제한다.
	void DisposeUploaders();

	// 정점/인덱스 버퍼를 바인딩하고 DrawIndexedInstanced를 호출한다.
	// 토폴로지와 PSO는 호출자가 미리 설정해야 한다.
	void Draw(ID3D12GraphicsCommandList* commandList) const;

	UINT GetIndexCount() const { return m_indexCount; }
	bool IsValid() const { return m_vertexBuffer != nullptr && m_indexBuffer != nullptr; }

private:
	// 디폴트 힙 버퍼를 만들고, 업로드 버퍼를 거쳐 복사하는 명령을 기록한다.
	static bool CreateDefaultBuffer(ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		const void* data,
		UINT64 byteSize,
		D3D12_RESOURCE_STATES finalState,
		ComPtr<ID3D12Resource>& outBuffer,
		ComPtr<ID3D12Resource>& outUploader,
		const wchar_t* debugName);

	ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_indexBuffer;

	// 복사가 끝날 때까지 살아 있어야 하는 임시 버퍼.
	ComPtr<ID3D12Resource> m_vertexUploader;
	ComPtr<ID3D12Resource> m_indexUploader;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};

	UINT m_indexCount = 0;
};
