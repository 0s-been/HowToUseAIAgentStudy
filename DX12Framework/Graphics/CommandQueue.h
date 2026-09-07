// CommandQueue.h
// 커맨드 큐 + 펜스(fence)를 한 덩어리로 묶은 클래스.
//
// D3D12에서 CPU와 GPU는 완전히 비동기로 돌아간다.
// CPU가 커맨드 리스트를 제출해도 GPU가 언제 그것을 끝낼지는 알 수 없기 때문에,
// "이 자원을 지금 건드려도 되는가?"를 판단하려면 펜스가 반드시 필요하다.
//
// 사용 흐름:
//   UINT64 v = queue.ExecuteCommandList(list);  // 제출 + 완료 표식(v) 예약
//   ... 다른 일 ...
//   queue.WaitForFenceValue(v);                 // v번 작업이 끝날 때까지 대기

#pragma once
#include "../Core/stdafx.h"

class CommandQueue
{
public:
	CommandQueue() = default;
	~CommandQueue();

	CommandQueue(const CommandQueue&) = delete;
	CommandQueue& operator=(const CommandQueue&) = delete;

	bool Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type, const wchar_t* debugName);
	void Shutdown();

	ID3D12CommandQueue* Get() const { return m_queue.Get(); }
	D3D12_COMMAND_LIST_TYPE GetType() const { return m_type; }

	// 커맨드 리스트는 호출 전에 Close되어 있어야 한다.
	// 반환값은 이 작업이 끝났음을 뜻하는 펜스 값이다.
	UINT64 ExecuteCommandList(ID3D12GraphicsCommandList* commandList);

	// 지금까지 제출된 작업 뒤에 표식을 하나 꽂고 그 값을 돌려준다.
	UINT64 Signal();

	// GPU가 해당 표식을 이미 지났는지 (대기 없이 확인).
	bool IsFenceComplete(UINT64 fenceValue) const;

	// 해당 표식을 지날 때까지 CPU를 블로킹한다.
	void WaitForFenceValue(UINT64 fenceValue);

	// 큐에 쌓인 모든 작업이 끝날 때까지 대기한다.
	// 리소스를 해제하거나 스왑체인을 재생성하기 직전에 반드시 호출해야 한다.
	void Flush();

private:
	ComPtr<ID3D12CommandQueue> m_queue;
	ComPtr<ID3D12Fence> m_fence;
	HANDLE m_fenceEvent = nullptr;

	// 다음에 발급할 펜스 값. 0은 "아직 아무 작업도 없음"을 뜻하도록 1부터 시작한다.
	UINT64 m_nextFenceValue = 1;

	D3D12_COMMAND_LIST_TYPE m_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
};
