// Renderer.h
// 디바이스 / 큐 / 스왑체인 / 프레임 자원을 한데 모아 프레임 흐름을 관리한다.
//
// 프레임 자원(FrameContext)을 백버퍼 장수만큼 두는 이유:
//   커맨드 얼로케이터는 GPU가 그 안의 명령을 다 실행하기 전에는 Reset할 수 없다.
//   얼로케이터가 하나뿐이면 CPU가 매 프레임 GPU를 기다려야 해서 병렬성이 사라진다.
//   장수만큼 두고 돌려쓰면 CPU는 N-1 프레임 앞서 나갈 수 있다.

#pragma once
#include "../Core/stdafx.h"
#include "CommandQueue.h"
#include "D3D12Device.h"
#include "SwapChain.h"

// 한 프레임이 배타적으로 쓰는 자원 묶음.
struct FrameContext
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	// 이 프레임이 마지막으로 제출한 작업의 펜스 값.
	// 같은 슬롯을 다시 쓰기 전에 이 값까지 GPU가 끝냈는지 확인한다.
	UINT64 fenceValue = 0;
};

class Renderer
{
public:
	Renderer() = default;
	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	bool Initialize(HWND hWnd, UINT width, UINT height, bool enableDebugLayer);
	void Shutdown();

	bool Resize(UINT width, UINT height);

	// BeginFrame ~ EndFrame 사이에서만 커맨드 리스트에 명령을 기록할 수 있다.
	void BeginFrame(const DirectX::XMFLOAT4& clearColor);
	void EndFrame(bool vsync);

	// 제출된 모든 GPU 작업이 끝날 때까지 대기한다.
	void WaitForGpu();

	D3D12Device& GetDevice() { return m_device; }
	CommandQueue& GetGraphicsQueue() { return m_graphicsQueue; }
	SwapChain& GetSwapChain() { return m_swapChain; }
	ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }

	float GetAspectRatio() const { return m_swapChain.GetAspectRatio(); }

protected:
	bool CreateFrameResources();

	D3D12Device m_device;
	CommandQueue m_graphicsQueue;
	SwapChain m_swapChain;

	FrameContext m_frames[kFrameBufferCount];
	ComPtr<ID3D12GraphicsCommandList> m_commandList;

	// 이번 프레임이 쓰는 FrameContext 슬롯. 백버퍼 인덱스와 같이 간다.
	UINT m_currentFrameIndex = 0;
	bool m_frameStarted = false;
};
