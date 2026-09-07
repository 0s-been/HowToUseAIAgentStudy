#include "SwapChain.h"

#include "../Core/DXException.h"
#include "../Core/Logger.h"
#include "CommandQueue.h"
#include "D3D12Device.h"
#include "D3D12Helpers.h"

SwapChain::~SwapChain()
{
	Shutdown();
}

bool SwapChain::Initialize(D3D12Device* device, CommandQueue* presentQueue, HWND hWnd, UINT width, UINT height)
{
	m_device = device;
	m_presentQueue = presentQueue;
	m_width = width;
	m_height = height;
	m_tearingSupported = device->IsTearingSupported();

	// 티어링을 쓰려면 스왑체인 생성 시점에 플래그를 넣어 두어야 한다.
	// 나중에 Present에서만 요청할 수는 없다.
	m_swapChainFlags = m_tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	// RTV는 백버퍼 장수만큼, DSV는 1개만 필요하다.
	if (!m_rtvHeap.Initialize(device->Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kFrameBufferCount, false, L"SwapChainRtvHeap"))
	{
		return false;
	}
	if (!m_dsvHeap.Initialize(device->Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false, L"SwapChainDsvHeap"))
	{
		return false;
	}

	m_rtvBaseIndex = m_rtvHeap.Allocate(kFrameBufferCount);
	m_dsvIndex = m_dsvHeap.Allocate(1);
	if (m_rtvBaseIndex == DescriptorHeap::kInvalidIndex || m_dsvIndex == DescriptorHeap::kInvalidIndex)
	{
		return false;
	}

	if (!CreateSwapChain(hWnd))
	{
		return false;
	}
	if (!CreateRenderTargetViews())
	{
		return false;
	}
	if (!CreateDepthStencilBuffer())
	{
		return false;
	}

	UpdateViewport();

	LOG_INFO(L"스왑체인 생성 완료: %u x %u, 백버퍼 %u장, 티어링 %s",
		m_width, m_height, kFrameBufferCount, m_tearingSupported ? L"허용" : L"미지원");
	return true;
}

void SwapChain::Shutdown()
{
	// 전체 화면 상태로 남아 있으면 스왑체인 해제가 실패한다.
	if (m_swapChain != nullptr)
	{
		m_swapChain->SetFullscreenState(FALSE, nullptr);
	}

	ReleaseSizeDependentResources();
	m_swapChain.Reset();
	m_rtvHeap.Shutdown();
	m_dsvHeap.Shutdown();
	m_device = nullptr;
	m_presentQueue = nullptr;
}

bool SwapChain::CreateSwapChain(HWND hWnd)
{
	try
	{
		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.Width = m_width;
		desc.Height = m_height;
		desc.Format = kBackBufferFormat;
		desc.Stereo = FALSE;
		// 플립 모델에서는 백버퍼에 MSAA를 직접 걸 수 없다.
		// MSAA가 필요하면 별도 렌더 타겟에 그린 뒤 Resolve해서 옮겨야 한다.
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = kFrameBufferCount;
		desc.Scaling = DXGI_SCALING_STRETCH;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		desc.Flags = m_swapChainFlags;

		// D3D12에서는 디바이스가 아니라 '커맨드 큐'를 넘긴다.
		ComPtr<IDXGISwapChain1> swapChain1;
		ThrowIfFailed(m_device->GetFactory()->CreateSwapChainForHwnd(
			m_presentQueue->Get(),
			hWnd,
			&desc,
			nullptr,	// 창 모드로 생성
			nullptr,
			&swapChain1));

		// Alt+Enter로 DXGI가 멋대로 전체 화면 전환을 하지 않도록 막는다.
		ThrowIfFailed(m_device->GetFactory()->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

		// GetCurrentBackBufferIndex는 IDXGISwapChain3부터 제공된다.
		ThrowIfFailed(swapChain1.As(&m_swapChain));
		m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
		return true;
	}
	catch (const DxException& e)
	{
		LOG_ERROR(L"스왑체인 생성 실패: %s", e.ToString().c_str());
		return false;
	}
}

bool SwapChain::CreateRenderTargetViews()
{
	try
	{
		for (UINT i = 0; i < kFrameBufferCount; ++i)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])));

			wchar_t name[32] = {};
			swprintf_s(name, L"BackBuffer[%u]", i);
			m_backBuffers[i]->SetName(name);

			// 리소스의 포맷을 그대로 쓰므로 설명자는 nullptr로 충분하다.
			m_device->Get()->CreateRenderTargetView(
				m_backBuffers[i].Get(), nullptr, m_rtvHeap.GetCpuHandle(m_rtvBaseIndex + i));
		}
		return true;
	}
	catch (const DxException& e)
	{
		LOG_ERROR(L"렌더 타겟 뷰 생성 실패: %s", e.ToString().c_str());
		return false;
	}
}

bool SwapChain::CreateDepthStencilBuffer()
{
	try
	{
		const D3D12_HEAP_PROPERTIES heapProps = DX::HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		const D3D12_RESOURCE_DESC desc = DX::Texture2DDesc(
			kDepthStencilFormat, m_width, m_height, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		// 클리어 값을 미리 알려주면 드라이버가 클리어를 더 빠르게 처리한다.
		// 실제 ClearDepthStencilView 값과 다르면 디버그 레이어가 경고를 낸다.
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = kDepthStencilFormat;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		ThrowIfFailed(m_device->Get()->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,	// 바로 깊이 기록 상태로 만든다.
			&clearValue,
			IID_PPV_ARGS(&m_depthStencilBuffer)));

		m_depthStencilBuffer->SetName(L"DepthStencilBuffer");

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = kDepthStencilFormat;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Texture2D.MipSlice = 0;

		m_device->Get()->CreateDepthStencilView(
			m_depthStencilBuffer.Get(), &dsvDesc, m_dsvHeap.GetCpuHandle(m_dsvIndex));
		return true;
	}
	catch (const DxException& e)
	{
		LOG_ERROR(L"깊이/스텐실 버퍼 생성 실패: %s", e.ToString().c_str());
		return false;
	}
}

void SwapChain::UpdateViewport()
{
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.Width = static_cast<float>(m_width);
	m_viewport.Height = static_cast<float>(m_height);
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = static_cast<LONG>(m_width);
	m_scissorRect.bottom = static_cast<LONG>(m_height);
}

void SwapChain::ReleaseSizeDependentResources()
{
	for (UINT i = 0; i < kFrameBufferCount; ++i)
	{
		m_backBuffers[i].Reset();
	}
	m_depthStencilBuffer.Reset();
}

bool SwapChain::Resize(UINT width, UINT height)
{
	if (width == 0 || height == 0)
	{
		return false;
	}
	if (width == m_width && height == m_height)
	{
		return true;
	}

	try
	{
		// GPU가 이전 백버퍼를 아직 쓰고 있을 수 있다. 반드시 먼저 기다린다.
		m_presentQueue->Flush();

		// ResizeBuffers는 백버퍼에 대한 참조가 하나도 남아 있지 않아야 성공한다.
		ReleaseSizeDependentResources();

		ThrowIfFailed(m_swapChain->ResizeBuffers(
			kFrameBufferCount, width, height, kBackBufferFormat, m_swapChainFlags));

		m_width = width;
		m_height = height;
		// 리사이즈 후에는 인덱스가 초기화되므로 다시 조회해야 한다.
		m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

		if (!CreateRenderTargetViews())
		{
			return false;
		}
		if (!CreateDepthStencilBuffer())
		{
			return false;
		}

		UpdateViewport();

		LOG_INFO(L"스왑체인 리사이즈 완료: %u x %u", m_width, m_height);
		return true;
	}
	catch (const DxException& e)
	{
		LOG_ERROR(L"스왑체인 리사이즈 실패: %s", e.ToString().c_str());
		return false;
	}
}

void SwapChain::Present(bool vsync)
{
	// 티어링은 창 모드 + vsync off일 때만 쓸 수 있다.
	const UINT syncInterval = vsync ? 1u : 0u;
	const UINT flags = (!vsync && m_tearingSupported) ? DXGI_PRESENT_ALLOW_TEARING : 0u;

	m_swapChain->Present(syncInterval, flags);

	// 다음 프레임이 그릴 백버퍼로 인덱스를 갱신한다.
	m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetCurrentBackBufferView() const
{
	return m_rtvHeap.GetCpuHandle(m_rtvBaseIndex + m_currentBackBufferIndex);
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetDepthStencilView() const
{
	return m_dsvHeap.GetCpuHandle(m_dsvIndex);
}

float SwapChain::GetAspectRatio() const
{
	return (m_height == 0) ? 1.0f : static_cast<float>(m_width) / static_cast<float>(m_height);
}
