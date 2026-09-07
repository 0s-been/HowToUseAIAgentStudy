#include "CommandQueue.h"

#include "../Core/DXException.h"
#include "../Core/Logger.h"

CommandQueue::~CommandQueue()
{
	Shutdown();
}

bool CommandQueue::Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type, const wchar_t* debugName)
{
	try
	{
		m_type = type;

		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = type;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_queue)));
		ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

		if (debugName != nullptr)
		{
			m_queue->SetName(debugName);
			m_fence->SetName(debugName);
		}

		// 펜스 완료를 기다릴 때 쓸 커널 이벤트. 수동 리셋이 아닌 자동 리셋 이벤트다.
		m_fenceEvent = ::CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
		if (m_fenceEvent == nullptr)
		{
			LOG_ERROR(L"펜스 이벤트 생성 실패. GetLastError = %lu", ::GetLastError());
			return false;
		}

		LOG_INFO(L"커맨드 큐 생성 완료: %s", (debugName != nullptr) ? debugName : L"(이름 없음)");
		return true;
	}
	catch (const DxException& e)
	{
		LOG_ERROR(L"커맨드 큐 생성 실패: %s", e.ToString().c_str());
		return false;
	}
}

void CommandQueue::Shutdown()
{
	// 큐가 살아 있다면 남은 작업을 모두 끝내고 정리한다.
	// 이 과정을 건너뛰면 GPU가 참조 중인 리소스를 해제하게 되어 크래시가 난다.
	if (m_queue != nullptr && m_fence != nullptr && m_fenceEvent != nullptr)
	{
		Flush();
	}

	if (m_fenceEvent != nullptr)
	{
		::CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}

	m_fence.Reset();
	m_queue.Reset();
}

UINT64 CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList* commandList)
{
	ID3D12CommandList* lists[] = { commandList };
	m_queue->ExecuteCommandLists(_countof(lists), lists);
	return Signal();
}

UINT64 CommandQueue::Signal()
{
	const UINT64 valueToSignal = m_nextFenceValue;

	// 큐에 이미 쌓인 작업이 전부 끝난 '뒤에' 펜스 값이 갱신된다.
	m_queue->Signal(m_fence.Get(), valueToSignal);

	++m_nextFenceValue;
	return valueToSignal;
}

bool CommandQueue::IsFenceComplete(UINT64 fenceValue) const
{
	return m_fence->GetCompletedValue() >= fenceValue;
}

void CommandQueue::WaitForFenceValue(UINT64 fenceValue)
{
	// 0은 "제출된 적 없음"을 뜻하므로 기다릴 필요가 없다.
	if (fenceValue == 0 || IsFenceComplete(fenceValue))
	{
		return;
	}

	// 지정한 값에 도달하면 이벤트가 신호 상태가 되도록 등록한 뒤 잠든다.
	m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
	::WaitForSingleObject(m_fenceEvent, INFINITE);
}

void CommandQueue::Flush()
{
	WaitForFenceValue(Signal());
}
