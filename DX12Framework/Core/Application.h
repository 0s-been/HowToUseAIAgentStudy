// Application.h
// 프로그램 전체의 수명주기(Initialize -> Run -> Shutdown)와 메인 루프를 담당한다.
// 창(Window)과 렌더러(Renderer)를 소유하고 둘을 이어 준다.
//
// 5단계 데모: 회전하는 큐브 하나를 그린다.
// 실제 게임/툴을 만들 때는 이 클래스가 씬이나 오브젝트 목록을 들고 있게 확장하면 된다.

#pragma once
#include "stdafx.h"
#include "Timer.h"
#include "Window.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Mesh.h"
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
	bool LoadResources();
	void Update(float deltaTime);
	void Render();
	void OnResize(UINT width, UINT height);
	void OnKeyDown(WPARAM key);
	void UpdateWindowTitle();

	HINSTANCE m_hInstance = nullptr;
	std::unique_ptr<Window> m_window;
	std::unique_ptr<Renderer> m_renderer;
	Timer m_timer;

	Camera m_camera;
	Mesh m_cubeMesh;

	// 데모용 큐브의 현재 회전각(라디안).
	float m_rotationY = 0.0f;
	float m_rotationX = 0.0f;
	bool m_rotationPaused = false;

	std::wstring m_baseTitle;
	float m_titleUpdateTimer = 0.0f;
	bool m_vsync = true;
	bool m_initialized = false;

	DirectX::XMFLOAT4 m_clearColor = { 0.08f, 0.10f, 0.16f, 1.0f };
};
