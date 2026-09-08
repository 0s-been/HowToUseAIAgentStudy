// SwapChain.h
// 스왑체인 + 백버퍼 RTV + 깊이/스텐실 버퍼 + 뷰포트를 한 덩어리로 관리한다.
//
// D3D11에서는 백버퍼가 사실상 1장처럼 보였지만, D3D12의 플립 모델에서는
// 백버퍼가 kFrameBufferCount장 있고 매 프레임 인덱스가 바뀐다.
// "지금 그릴 대상"을 얻으려면 반드시 GetCurrentBackBufferIndex를 거쳐야 한다.
//
// [MSAA]
// 플립 모델 백버퍼는 멀티샘플을 직접 지원하지 않는다. 그래서 하드웨어가 지원하면
// 별도의 멀티샘플 색상 타겟(m_msaaColorTarget)에 그린 뒤, 프레임 끝에서 그 내용을
// ResolveSubresource로 백버퍼에 내려받는다(다운샘플). 지원하지 않으면 예전처럼
// 백버퍼에 직접 그린다 - GetColorTargetView()가 그 차이를 감춘다.

#pragma once
#include "../Core/stdafx.h"
#include "DescriptorHeap.h"

class D3D12Device;
class CommandQueue;

class SwapChain
{
public:
	static constexpr DXGI_FORMAT kBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT kDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// 하드웨어가 지원하면 이 샘플 수로 MSAA를 켠다. 지원하지 않으면 1(끔)로 자동 낮춘다.
	static constexpr UINT kDesiredMsaaSampleCount = 4;

	SwapChain() = default;
	~SwapChain();

	SwapChain(const SwapChain&) = delete;
	SwapChain& operator=(const SwapChain&) = delete;

	// presentQueue는 이 스왑체인이 Present를 수행할 큐다. (반드시 DIRECT 타입)
	bool Initialize(D3D12Device* device, CommandQueue* presentQueue, HWND hWnd, UINT width, UINT height);
	void Shutdown();

	// 창 크기가 바뀌었을 때 호출한다. 내부에서 GPU 작업 완료를 기다린 뒤 버퍼를 재생성한다.
	bool Resize(UINT width, UINT height);

	// vsync가 false이고 하드웨어가 티어링을 지원하면 프레임 상한 없이 표시한다.
	void Present(bool vsync);

	UINT GetCurrentBackBufferIndex() const { return m_currentBackBufferIndex; }
	ID3D12Resource* GetCurrentBackBuffer() const { return m_backBuffers[m_currentBackBufferIndex].Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

	const D3D12_VIEWPORT& GetViewport() const { return m_viewport; }
	const D3D12_RECT& GetScissorRect() const { return m_scissorRect; }

	UINT GetWidth() const { return m_width; }
	UINT GetHeight() const { return m_height; }
	float GetAspectRatio() const;

	// 실제로 적용된 샘플 수/품질. 하드웨어가 지원하지 않으면 1/0(MSAA 꺼짐)이다.
	UINT GetSampleCount() const { return m_sampleCount; }
	UINT GetSampleQuality() const { return m_msaaQuality; }
	bool IsMsaaEnabled() const { return m_sampleCount > 1; }

	// 실제로 그리기 명령이 향해야 하는 렌더 타겟의 뷰.
	// MSAA가 꺼져 있으면 GetCurrentBackBufferView()와 같은 값이다.
	D3D12_CPU_DESCRIPTOR_HANDLE GetColorTargetView() const;
	ID3D12Resource* GetMsaaColorTarget() const { return m_msaaColorTarget.Get(); }

private:
	bool CreateSwapChain(HWND hWnd);
	bool CreateRenderTargetViews();
	bool CreateMsaaColorTarget();
	bool CreateDepthStencilBuffer();
	void UpdateViewport();
	void ReleaseSizeDependentResources();

	D3D12Device* m_device = nullptr;
	CommandQueue* m_presentQueue = nullptr;

	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Resource> m_backBuffers[kFrameBufferCount];
	ComPtr<ID3D12Resource> m_msaaColorTarget;
	ComPtr<ID3D12Resource> m_depthStencilBuffer;

	DescriptorHeap m_rtvHeap;
	DescriptorHeap m_dsvHeap;
	// MSAA 색상 타겟 전용 힙. 백버퍼 RTV 힙과 분리한 이유는 크기가 다르기 때문이다
	// (백버퍼는 kFrameBufferCount장, MSAA 타겟은 프레임 수와 무관하게 딱 1장).
	DescriptorHeap m_msaaRtvHeap;
	UINT m_rtvBaseIndex = DescriptorHeap::kInvalidIndex;
	UINT m_dsvIndex = DescriptorHeap::kInvalidIndex;
	UINT m_msaaRtvIndex = DescriptorHeap::kInvalidIndex;

	UINT m_sampleCount = 1;
	UINT m_msaaQuality = 0;

	D3D12_VIEWPORT m_viewport = {};
	D3D12_RECT m_scissorRect = {};

	UINT m_width = 0;
	UINT m_height = 0;
	UINT m_currentBackBufferIndex = 0;
	UINT m_swapChainFlags = 0;
	bool m_tearingSupported = false;
};
