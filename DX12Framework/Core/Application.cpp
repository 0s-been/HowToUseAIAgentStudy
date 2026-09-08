#include "Application.h"

#include "Logger.h"

using namespace DirectX;

Application::Application(HINSTANCE hInstance)
	: m_hInstance(hInstance)
{
}

Application::~Application()
{
	Shutdown();
}

bool Application::Initialize(const std::wstring& title, UINT width, UINT height)
{
	m_baseTitle = title;

	m_window = std::make_unique<Window>(m_hInstance, title, width, height);

	// 창이 D3D를 직접 알지 못하도록, 크기 변경은 콜백으로만 받는다.
	m_window->SetResizeCallback([this](UINT w, UINT h) { OnResize(w, h); });

	if (!m_window->Create())
	{
		return false;
	}

	// 창이 받은 원시 메시지를 입력 리더로 흘려보낸다.
	// Window는 입력의 의미를 모르고, InputReader는 창 생성을 모른다.
	m_input = std::make_unique<InputReader>();
	m_input->Initialize(m_window->GetHandle());
	m_window->SetMessageCallback([this](UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (m_input != nullptr)
		{
			m_input->ProcessMessage(message, wParam, lParam);
		}
	});

	// 디버그 레이어는 디버그 빌드에서만 켠다. 릴리스에서는 비용이 크다.
#if defined(_DEBUG)
	constexpr bool kEnableDebugLayer = true;
#else
	constexpr bool kEnableDebugLayer = false;
#endif

	m_renderer = std::make_unique<Renderer>();
	if (!m_renderer->Initialize(m_window->GetHandle(), width, height, kEnableDebugLayer))
	{
		LOG_ERROR(L"렌더러 초기화 실패");
		return false;
	}

	if (!LoadResources())
	{
		return false;
	}

	// 큐브가 화면에 알맞게 들어오는 위치에서 원점을 바라본다.
	m_camera.SetLens(XMConvertToRadians(60.0f), m_renderer->GetAspectRatio(), 0.1f, 500.0f);
	m_camera.LookAt(
		XMFLOAT3(4.0f, 3.5f, -7.0f),	// 위치
		XMFLOAT3(0.0f, 1.0f, 0.0f));	// 바라보는 지점 (큐브 높이)

	m_renderer->SetDirectionalLight(
		XMFLOAT3(0.5f, -1.0f, 0.75f),		// 빛이 나아가는 방향
		XMFLOAT4(1.0f, 0.97f, 0.90f, 1.0f),	// 광원 색
		XMFLOAT4(0.22f, 0.24f, 0.30f, 1.0f));	// 환경광

	m_timer.Reset();
	m_initialized = true;

	LOG_INFO(L"애플리케이션 초기화 완료");
	LOG_INFO(L"조작: WASD 이동 / QE 상하 / Shift 가속 / 우클릭 드래그 시야 / Space 회전정지 / V 수직동기화 / ESC 종료");
	return true;
}

bool Application::LoadResources()
{
	if (!m_meshFactory.Initialize(m_renderer.get()))
	{
		return false;
	}

	// 40 x 40 크기를 20 x 20칸으로 나눈 바닥. m_groundCellSize(2.0f)와 맞춰야 한다.
	// 칸 하나가 2단위라 카메라가 얼마나 움직였는지 눈으로 가늠할 수 있다.
	// 메시 자체는 흰색 정점만 가지고 있고, 체커 무늬는 Render()에서 DrawMesh에
	// 색을 넘겨 픽셀 셰이더가 그린다 (화면 공간 도함수로 앤티앨리어싱된다).
	m_groundMesh = m_meshFactory.CreateGrid(L"GroundGrid", 40.0f, 40.0f, 20, 20);
	if (m_groundMesh == nullptr)
	{
		return false;
	}

	m_cubeMesh = m_meshFactory.CreateBox(L"Cube", 1.5f, 1.5f, 1.5f);
	if (m_cubeMesh == nullptr)
	{
		return false;
	}

	LOG_INFO(L"리소스 로드 완료 (메시 %zu개)", m_meshFactory.GetCount());
	return true;
}

int Application::Run()
{
	if (!m_initialized)
	{
		return -1;
	}

	m_timer.Reset();

	while (true)
	{
		// 메시지 펌프보다 '먼저' 호출해야 한다.
		// 여기서 지난 프레임 상태를 보관하고 마우스 이동량을 0으로 되돌린 뒤,
		// 이어지는 ProcessMessages가 이번 프레임의 입력을 채운다.
		m_input->BeginFrame();

		if (!m_window->ProcessMessages())
		{
			break;
		}

		m_timer.Tick();

		// 최소화되었거나 비활성 상태면 그리지 않고 CPU를 양보한다.
		if (m_window->IsPaused() || m_window->IsMinimized())
		{
			::Sleep(100);
			continue;
		}

		Update(m_timer.GetDeltaTime());
		Render();
		UpdateWindowTitle();
	}

	return 0;
}

void Application::Shutdown()
{
	if (m_renderer != nullptr)
	{
		// 메시를 파괴하기 전에 GPU 작업이 모두 끝나야 한다.
		m_renderer->WaitForGpu();

		// 팩토리가 보유한 모든 메시를 해제한다. 이후 관찰 포인터는 무효다.
		m_meshFactory.Shutdown();
		m_cubeMesh = nullptr;
		m_groundMesh = nullptr;

		m_renderer.reset();
	}

	// 순서가 중요하다. Window를 파괴하면 DestroyWindow가 WM_DESTROY를 보내고,
	// 그 메시지가 콜백을 타고 이미 해제된 m_input에 닿으면 크래시가 난다.
	// 콜백부터 끊고 창을 먼저 정리한다.
	if (m_window != nullptr)
	{
		m_window->SetMessageCallback(nullptr);
	}
	m_window.reset();
	m_input.reset();
	m_initialized = false;
}

void Application::Update(float deltaTime)
{
	// 토글류는 '눌린 그 프레임'에만 반응해야 한다.
	// IsKeyDown으로 처리하면 키를 누르고 있는 동안 매 프레임 뒤집힌다.
	if (m_input->WasKeyPressed('V'))
	{
		m_vsync = !m_vsync;
		LOG_INFO(L"수직 동기화: %s", m_vsync ? L"켜짐" : L"꺼짐");
	}
	if (m_input->WasKeyPressed(VK_SPACE))
	{
		m_rotationPaused = !m_rotationPaused;
		LOG_INFO(L"큐브 회전: %s", m_rotationPaused ? L"정지" : L"재개");
	}
	if (m_input->WasKeyPressed(VK_TAB))
	{
		m_wireframeEnabled = !m_wireframeEnabled;
		m_renderer->SetWireframe(m_wireframeEnabled);
		LOG_INFO(L"와이어프레임: %s", m_wireframeEnabled ? L"켜짐" : L"꺼짐");
	}

	UpdateCamera(deltaTime);

	if (m_rotationPaused)
	{
		return;
	}

	// 프레임 수가 아니라 경과 시간에 비례해 회전시킨다.
	// 그래야 fps가 달라져도 회전 속도가 같다.
	m_rotationY += deltaTime * XMConvertToRadians(45.0f);
	m_rotationX += deltaTime * XMConvertToRadians(20.0f);

	// 값이 무한정 커지면서 정밀도가 떨어지는 것을 막는다.
	m_rotationY = XMScalarModAngle(m_rotationY);
	m_rotationX = XMScalarModAngle(m_rotationX);
}

void Application::Render()
{
	m_renderer->BeginFrame(m_clearColor);
	m_renderer->SetPassConstants(m_camera, m_timer.GetTotalTime());

	// 바닥은 원점에 그대로 놓는다. 격자 자체가 이미 XZ 평면 위에 만들어져 있다.
	// 체커 무늬는 정점 색이 아니라 픽셀 셰이더에서 화면 공간 도함수로 계산되어
	// 먼 거리에서도(칸이 화면 1픽셀보다 작아져도) 어른거리지 않는다.
	m_renderer->DrawMesh(*m_groundMesh, XMMatrixIdentity(), m_groundColorA, m_groundColorB, m_groundCellSize);

	// 큐브는 바닥에 파묻히지 않도록 반 높이만큼 띄운 뒤 회전시킨다.
	const XMMATRIX cubeWorld =
		XMMatrixRotationX(m_rotationX) *
		XMMatrixRotationY(m_rotationY) *
		XMMatrixTranslation(0.0f, 1.2f, 0.0f);
	m_renderer->DrawMesh(*m_cubeMesh, cubeWorld, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

	m_renderer->EndFrame(m_vsync);
}

void Application::OnResize(UINT width, UINT height)
{
	if (m_renderer == nullptr)
	{
		return;
	}

	if (m_renderer->Resize(width, height))
	{
		// 종횡비가 바뀌었으므로 투영 행렬을 다시 만든다.
		// 이걸 빠뜨리면 창을 늘렸을 때 물체가 찌그러진다.
		m_camera.SetAspectRatio(m_renderer->GetAspectRatio());
	}
}

void Application::UpdateCamera(float deltaTime)
{
	// 마우스 오른쪽 버튼을 누르고 있는 동안에만 시야가 돈다.
	// 버튼을 떼면 커서로 다른 작업을 할 수 있어야 하기 때문이다.
	if (m_input->IsMouseDown(MouseButton::Right))
	{
		// 회전량은 마우스가 움직인 '픽셀 수'에 비례한다.
		// 여기에 deltaTime을 곱하면 안 된다. 이동량 자체가 이미 이번 프레임의 값이라
		// 곱하는 순간 프레임률에 따라 감도가 달라진다.
		const float deltaX = m_input->GetMouseDeltaX();
		const float deltaY = m_input->GetMouseDeltaY();

		// 아주 빠른 마우스 동작은 픽셀 델타 자체가 커서, 감도를 곱한 뒤에도
		// 한 프레임 회전량이 비정상적으로 커질 수 있다. 여기서 한 번 더 잘라 둔다.
		const float yaw = std::clamp(deltaX * m_mouseSensitivity, -m_maxRotationPerFrame, m_maxRotationPerFrame);
		const float pitch = std::clamp(deltaY * m_mouseSensitivity, -m_maxRotationPerFrame, m_maxRotationPerFrame);

		if (yaw != 0.0f)
		{
			m_camera.AddYaw(yaw);
		}
		if (pitch != 0.0f)
		{
			// 마우스를 아래로 끌면 아래를 본다.
			m_camera.AddPitch(pitch);
		}
	}

	// 반대로 이동은 '시간'에 비례해야 프레임률과 무관하게 같은 속도가 된다.
	float speed = m_cameraSpeed;
	if (m_input->IsKeyDown(VK_SHIFT))
	{
		speed *= m_cameraBoostMultiplier;
	}
	const float distance = speed * deltaTime;

	if (m_input->IsKeyDown('W')) { m_camera.Walk(distance); }
	if (m_input->IsKeyDown('S')) { m_camera.Walk(-distance); }
	if (m_input->IsKeyDown('D')) { m_camera.Strafe(distance); }
	if (m_input->IsKeyDown('A')) { m_camera.Strafe(-distance); }
	if (m_input->IsKeyDown('E')) { m_camera.Fly(distance); }
	if (m_input->IsKeyDown('Q')) { m_camera.Fly(-distance); }

	// 이동/회전으로 더러워진 뷰 행렬을 여기서 한 번만 다시 만든다.
	m_camera.UpdateViewMatrix();
}

void Application::UpdateWindowTitle()
{
	m_titleUpdateTimer += m_timer.GetDeltaTime();
	if (m_titleUpdateTimer < 1.0f)
	{
		return;
	}
	m_titleUpdateTimer = 0.0f;

	wchar_t title[192] = {};
	swprintf_s(title, L"%s    fps: %.0f    %.2f ms    vsync: %s",
		m_baseTitle.c_str(), m_timer.GetFps(), m_timer.GetMsPerFrame(), m_vsync ? L"on" : L"off");
	m_window->SetTitle(title);
}
