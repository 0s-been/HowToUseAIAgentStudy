#include "Application.h"

#include "Logger.h"
#include "../Graphics/GeometryFactory.h"

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
	m_window->SetKeyDownCallback([this](WPARAM key) { OnKeyDown(key); });

	if (!m_window->Create())
	{
		return false;
	}

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
		XMFLOAT3(2.5f, 2.0f, -4.0f),	// 위치
		XMFLOAT3(0.0f, 0.0f, 0.0f),		// 바라보는 지점
		XMFLOAT3(0.0f, 1.0f, 0.0f));	// 위쪽

	m_renderer->SetDirectionalLight(
		XMFLOAT3(0.5f, -1.0f, 0.75f),		// 빛이 나아가는 방향
		XMFLOAT4(1.0f, 0.97f, 0.90f, 1.0f),	// 광원 색
		XMFLOAT4(0.22f, 0.24f, 0.30f, 1.0f));	// 환경광

	m_timer.Reset();
	m_initialized = true;

	LOG_INFO(L"애플리케이션 초기화 완료 (V: 수직동기화, Space: 회전 정지, ESC: 종료)");
	return true;
}

bool Application::LoadResources()
{
	const MeshData boxData = GeometryFactory::CreateBox(1.5f, 1.5f, 1.5f);
	if (!m_renderer->CreateMesh(m_cubeMesh, boxData, L"CubeMesh"))
	{
		LOG_ERROR(L"큐브 메시 생성 실패");
		return false;
	}
	return true;
}

int Application::Run()
{
	if (!m_initialized)
	{
		return -1;
	}

	m_timer.Reset();

	while (m_window->ProcessMessages())
	{
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
		m_cubeMesh.Shutdown();
		m_renderer.reset();
	}

	m_window.reset();
	m_initialized = false;
}

void Application::Update(float deltaTime)
{
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

	const XMMATRIX world = XMMatrixRotationX(m_rotationX) * XMMatrixRotationY(m_rotationY);
	m_renderer->DrawMesh(m_cubeMesh, world, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

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

void Application::OnKeyDown(WPARAM key)
{
	switch (key)
	{
		case 'V':
			m_vsync = !m_vsync;
			LOG_INFO(L"수직 동기화: %s", m_vsync ? L"켜짐" : L"꺼짐");
			break;

		case VK_SPACE:
			m_rotationPaused = !m_rotationPaused;
			LOG_INFO(L"회전: %s", m_rotationPaused ? L"정지" : L"재개");
			break;

		default:
			break;
	}
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
