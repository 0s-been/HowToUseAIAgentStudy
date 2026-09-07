#include "Renderer.h"

#include "../Core/DXException.h"
#include "../Core/Logger.h"
#include "D3D12Helpers.h"

Renderer::~Renderer()
{
	Shutdown();
}

bool Renderer::Initialize(HWND hWnd, UINT width, UINT height, bool enableDebugLayer)
{
	if (!m_device.Initialize(enableDebugLayer))
	{
		return false;
	}

	if (!m_graphicsQueue.Initialize(m_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, L"GraphicsQueue"))
	{
		return false;
	}

	if (!m_swapChain.Initialize(&m_device, &m_graphicsQueue, hWnd, width, height))
	{
		return false;
	}

	if (!CreateFrameResources())
	{
		return false;
	}

	m_currentFrameIndex = m_swapChain.GetCurrentBackBufferIndex();

	LOG_INFO(L"렌더러 초기화 완료");
	return true;
}

bool Renderer::CreateFrameResources()
{
	try
	{
		for (UINT i = 0; i < kFrameBufferCount; ++i)
		{
			ThrowIfFailed(m_device.Get()->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_frames[i].commandAllocator)));

			wchar_t name[40] = {};
			swprintf_s(name, L"CommandAllocator[%u]", i);
			m_frames[i].commandAllocator->SetName(name);

			m_frames[i].fenceValue = 0;
		}

		// 커맨드 리스트는 1개만 두고 매 프레임 얼로케이터만 바꿔가며 Reset한다.
		// (기록은 CPU 한 스레드에서만 하므로 여러 개가 필요 없다.)
		ThrowIfFailed(m_device.Get()->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_frames[0].commandAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(&m_commandList)));
		m_commandList->SetName(L"MainCommandList");

		// CreateCommandList는 '기록 중' 상태로 만들어 준다.
		// BeginFrame이 항상 Reset으로 시작할 수 있도록 여기서 한 번 닫아 둔다.
		ThrowIfFailed(m_commandList->Close());

		LOG_INFO(L"프레임 자원 생성 완료 (커맨드 얼로케이터 %u개)", kFrameBufferCount);
		return true;
	}
	catch (const DxException& e)
	{
		LOG_ERROR(L"프레임 자원 생성 실패: %s", e.ToString().c_str());
		return false;
	}
}

void Renderer::Shutdown()
{
	// 어떤 것도 해제하기 전에 GPU를 반드시 비운다.
	if (m_graphicsQueue.Get() != nullptr)
	{
		m_graphicsQueue.Flush();
	}

	m_commandList.Reset();
	for (UINT i = 0; i < kFrameBufferCount; ++i)
	{
		m_frames[i].commandAllocator.Reset();
		m_frames[i].fenceValue = 0;
	}

	m_swapChain.Shutdown();
	m_graphicsQueue.Shutdown();
	m_device.Shutdown();
}

void Renderer::WaitForGpu()
{
	m_graphicsQueue.Flush();

	// 큐를 비웠으므로 모든 프레임 슬롯을 다시 써도 안전하다.
	for (UINT i = 0; i < kFrameBufferCount; ++i)
	{
		m_frames[i].fenceValue = 0;
	}
}

bool Renderer::Resize(UINT width, UINT height)
{
	// SwapChain::Resize가 내부에서 Flush하지만, 프레임 슬롯의 펜스 값도 함께 정리해야 한다.
	WaitForGpu();
	return m_swapChain.Resize(width, height);
}

void Renderer::BeginFrame(const DirectX::XMFLOAT4& clearColor)
{
	m_currentFrameIndex = m_swapChain.GetCurrentBackBufferIndex();
	FrameContext& frame = m_frames[m_currentFrameIndex];

	// 이 슬롯을 마지막으로 쓴 프레임이 GPU에서 끝났는지 확인한다.
	// 끝나지 않았다면 여기서 대기한다. (CPU가 GPU보다 너무 앞서가는 것을 막는 지점)
	m_graphicsQueue.WaitForFenceValue(frame.fenceValue);

	// 얼로케이터를 Reset하면 이전에 기록된 명령이 통째로 버려진다.
	frame.commandAllocator->Reset();
	m_commandList->Reset(frame.commandAllocator.Get(), nullptr);

	// 백버퍼를 '표시용'에서 '렌더 타겟'으로 전이시킨다.
	// 이 배리어를 빠뜨리면 디버그 레이어가 즉시 오류를 낸다.
	const D3D12_RESOURCE_BARRIER toRenderTarget = DX::TransitionBarrier(
		m_swapChain.GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &toRenderTarget);

	// 뷰포트와 가위 영역은 커맨드 리스트를 Reset할 때마다 초기화되므로 매 프레임 다시 설정한다.
	const D3D12_VIEWPORT viewport = m_swapChain.GetViewport();
	const D3D12_RECT scissor = m_swapChain.GetScissorRect();
	m_commandList->RSSetViewports(1, &viewport);
	m_commandList->RSSetScissorRects(1, &scissor);

	const D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain.GetCurrentBackBufferView();
	const D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_swapChain.GetDepthStencilView();
	m_commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	const float color[4] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
	m_commandList->ClearRenderTargetView(rtv, color, 0, nullptr);
	m_commandList->ClearDepthStencilView(
		dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_frameStarted = true;
}

void Renderer::EndFrame(bool vsync)
{
	if (!m_frameStarted)
	{
		return;
	}

	FrameContext& frame = m_frames[m_currentFrameIndex];

	// 화면에 내보내려면 다시 PRESENT 상태여야 한다.
	const D3D12_RESOURCE_BARRIER toPresent = DX::TransitionBarrier(
		m_swapChain.GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &toPresent);

	m_commandList->Close();

	// 제출과 동시에 이 프레임의 완료 표식을 받아 둔다.
	// 다음에 같은 슬롯을 쓸 때 이 값을 기다리게 된다.
	frame.fenceValue = m_graphicsQueue.ExecuteCommandList(m_commandList.Get());

	m_swapChain.Present(vsync);
	m_frameStarted = false;
}
