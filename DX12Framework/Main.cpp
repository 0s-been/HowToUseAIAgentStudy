// Main.cpp
// DX12Framework 진입점.
//
// [2단계] 창 + 타이머/로거에 더해 D3D12 디바이스와 커맨드 큐 생성까지 확인한다.
//         이후 단계에서 Application 클래스로 옮겨 렌더러와 연결한다.

#include "Core/stdafx.h"
#include "Core/Logger.h"
#include "Core/Timer.h"
#include "Core/Window.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/D3D12Device.h"

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nShowCmd);

	Logger::Get().Initialize(L"DX12Framework.log", true);
	LOG_INFO(L"===== DX12Framework 시작 =====");

	Window window(hInstance, L"DX12 Framework", 1280, 720);

	window.SetResizeCallback([](UINT width, UINT height)
	{
		LOG_INFO(L"윈도우 크기 변경: %u x %u", width, height);
	});

	if (!window.Create())
	{
		LOG_ERROR(L"윈도우 생성에 실패해 종료한다.");
		Logger::Get().Shutdown();
		return -1;
	}

	// 디버그 레이어는 디버그 빌드에서만 켠다. 릴리스에서는 비용이 크다.
#if defined(_DEBUG)
	constexpr bool kEnableDebugLayer = true;
#else
	constexpr bool kEnableDebugLayer = false;
#endif

	D3D12Device device;
	if (!device.Initialize(kEnableDebugLayer))
	{
		LOG_ERROR(L"D3D12 디바이스 초기화에 실패해 종료한다.");
		Logger::Get().Shutdown();
		return -1;
	}

	CommandQueue graphicsQueue;
	if (!graphicsQueue.Initialize(device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, L"GraphicsQueue"))
	{
		LOG_ERROR(L"커맨드 큐 초기화에 실패해 종료한다.");
		Logger::Get().Shutdown();
		return -1;
	}

	Timer timer;
	timer.Reset();

	float titleUpdateTimer = 0.0f;

	while (window.ProcessMessages())
	{
		timer.Tick();

		if (window.IsPaused())
		{
			// 비활성/최소화 상태에서는 CPU를 양보한다.
			::Sleep(100);
			continue;
		}

		// 1초에 한 번 타이틀바에 FPS를 표시한다.
		titleUpdateTimer += timer.GetDeltaTime();
		if (titleUpdateTimer >= 1.0f)
		{
			titleUpdateTimer = 0.0f;

			wchar_t title[128] = {};
			swprintf_s(title, L"DX12 Framework    fps: %.0f    %.2f ms",
				timer.GetFps(), timer.GetMsPerFrame());
			window.SetTitle(title);
		}
	}

	// 리소스를 해제하기 전에 GPU가 모든 작업을 끝내도록 반드시 기다린다.
	graphicsQueue.Flush();

	LOG_INFO(L"===== DX12Framework 종료 (총 실행 시간 %.1f초) =====", timer.GetTotalTime());

	graphicsQueue.Shutdown();
	device.Shutdown();
	Logger::Get().Shutdown();

	// 해제되지 않은 D3D 객체가 있으면 출력 창에 보고된다.
	D3D12Device::ReportLiveObjects();
	return 0;
}
