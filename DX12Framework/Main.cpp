// Main.cpp
// DX12Framework 진입점. 실제 동작은 전부 Application이 담당한다.

#include "Core/stdafx.h"
#include "Core/Application.h"
#include "Core/Logger.h"
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

	int exitCode = 0;
	{
		Application app(hInstance);

		if (app.Initialize(L"DX12 Framework", 1280, 720))
		{
			exitCode = app.Run();
		}
		else
		{
			LOG_ERROR(L"초기화에 실패해 종료한다.");
			exitCode = -1;
		}

		// app이 이 블록을 벗어나며 소멸한다.
		// 아래 ReportLiveObjects보다 먼저 정리되어야 누수 보고가 정확해진다.
	}

	LOG_INFO(L"===== DX12Framework 종료 =====");
	Logger::Get().Shutdown();

	// 해제되지 않은 D3D 객체가 있으면 출력 창에 보고된다.
	D3D12Device::ReportLiveObjects();
	return exitCode;
}
