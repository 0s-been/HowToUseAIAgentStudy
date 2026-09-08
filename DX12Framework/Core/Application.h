// Application.h
// 프로그램 전체의 수명주기(Initialize -> Run -> Shutdown)와 메인 루프를 담당한다.
// 창(Window)과 렌더러(Renderer)를 소유하고 둘을 이어 준다.
//
// 데모: 체커 무늬 격자 바닥 위에 회전하는 큐브를 그리고,
//       WASD + 우클릭 드래그로 카메라를 움직인다.
// 메시는 직접 만들지 않고 전부 MeshFactory를 통해 얻는다.
// Tab으로 솔리드/와이어프레임을 토글할 수 있다.
// 실제 게임/툴을 만들 때는 이 클래스가 씬이나 오브젝트 목록을 들고 있게 확장하면 된다.

#pragma once
#include "stdafx.h"
#include "Timer.h"
#include "Window.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/MeshFactory.h"
#include "../Graphics/Renderer.h"
#include "../Input/InputReader.h"

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
	void UpdateCamera(float deltaTime);
	void Render();
	void OnResize(UINT width, UINT height);
	void UpdateWindowTitle();

	HINSTANCE m_hInstance = nullptr;
	std::unique_ptr<Window> m_window;
	std::unique_ptr<Renderer> m_renderer;
	std::unique_ptr<InputReader> m_input;
	Timer m_timer;

	Camera m_camera;

	// 메시의 소유자는 팩토리다. 아래 포인터는 관찰용이며 해제하지 않는다.
	MeshFactory m_meshFactory;
	Mesh* m_cubeMesh = nullptr;
	Mesh* m_groundMesh = nullptr;

	// 바닥 체커 무늬의 두 색과 칸 크기(월드 단위). 메시 자체는 색을 갖지 않으므로
	// 그릴 때(DrawMesh) 넘겨준다. 칸 크기는 격자를 만들 때 쓴 값(40/20)과 맞춰야 한다.
	DirectX::XMFLOAT4 m_groundColorA = { 0.62f, 0.64f, 0.68f, 1.0f };
	DirectX::XMFLOAT4 m_groundColorB = { 0.42f, 0.44f, 0.49f, 1.0f };
	float m_groundCellSize = 2.0f;

	// 데모용 큐브의 현재 회전각(라디안).
	float m_rotationY = 0.0f;
	float m_rotationX = 0.0f;
	bool m_rotationPaused = false;
	bool m_wireframeEnabled = false;

	// 카메라 이동 속도(초당 단위 거리)와 Shift를 눌렀을 때의 배율.
	float m_cameraSpeed = 5.0f;
	float m_cameraBoostMultiplier = 3.0f;
	// 마우스 1픽셀당 회전 각도. 값이 클수록 시야가 빨리 돈다.
	float m_mouseSensitivity = DirectX::XMConvertToRadians(0.25f);

	std::wstring m_baseTitle;
	float m_titleUpdateTimer = 0.0f;
	bool m_vsync = true;
	bool m_initialized = false;

	DirectX::XMFLOAT4 m_clearColor = { 0.08f, 0.10f, 0.16f, 1.0f };
};
