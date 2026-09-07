// Main.cpp
// DX12Framework 진입점.
//
// [1단계] 현재는 창 생성 + 메시지 루프 + 타이머/로거 동작 확인까지만 담당한다.
//         이후 단계에서 Application 클래스로 옮겨 렌더러와 연결한다.

#include "Core/stdafx.h"
#include "Core/Logger.h"
#include "Core/Timer.h"
#include "Core/Window.h"

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

	LOG_INFO(L"===== DX12Framework 종료 (총 실행 시간 %.1f초) =====", timer.GetTotalTime());
	Logger::Get().Shutdown();
	return 0;
}
