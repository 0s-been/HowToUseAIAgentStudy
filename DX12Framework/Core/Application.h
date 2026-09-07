// Application.h
// 프로그램 전체의 수명주기(Initialize -> Run -> Shutdown)와 메인 루프를 담당한다.
// 창(Window)과 렌더러(Renderer)를 소유하고 둘을 이어 준다.

#pragma once
#include "stdafx.h"
#include "Timer.h"
#include "Window.h"
#include "../Graphics/Renderer.h"

class Application
{
public:
	explicit Application(HINSTANCE hInstance);
	~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	bool Initialize(const std::wstring& title, UINT width, UINT height);
	int Run();
	void Shutdown();

private:
	void Update(float deltaTime);
	void Render();
	void OnResize(UINT width, UINT height);
	void OnKeyDown(WPARAM key);
	void UpdateWindowTitle();

	HINSTANCE m_hInstance = nullptr;
	std::unique_ptr<Window> m_window;
	std::unique_ptr<Renderer> m_renderer;
	Timer m_timer;

	std::wstring m_baseTitle;
	float m_titleUpdateTimer = 0.0f;
	bool m_vsync = true;
	bool m_initialized = false;

	DirectX::XMFLOAT4 m_clearColor = { 0.1f, 0.15f, 0.25f, 1.0f };
};
