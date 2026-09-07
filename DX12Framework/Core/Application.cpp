#include "Application.h"

#include "Logger.h"

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

	m_timer.Reset();
	m_initialized = true;

	LOG_INFO(L"애플리케이션 초기화 완료");
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
		// 렌더러를 파괴하기 전에 GPU 작업이 모두 끝나야 한다.
		m_renderer->WaitForGpu();
		m_renderer.reset();
	}

	m_window.reset();
	m_initialized = false;
}

void Application::Update(float deltaTime)
{
	UNREFERENCED_PARAMETER(deltaTime);
	// 3단계에서는 아직 그릴 것이 없다. 다음 단계에서 카메라와 오브젝트가 들어온다.
}

void Application::Render()
{
	m_renderer->BeginFrame(m_clearColor);
	// 이 사이에 그리기 명령이 들어간다.
	m_renderer->EndFrame(m_vsync);
}

void Application::OnResize(UINT width, UINT height)
{
	if (m_renderer == nullptr)
	{
		return;
	}

	m_renderer->Resize(width, height);
}

void Application::OnKeyDown(WPARAM key)
{
	// V키로 수직 동기화를 껐다 켠다.
	if (key == 'V')
	{
		m_vsync = !m_vsync;
		LOG_INFO(L"수직 동기화: %s", m_vsync ? L"켜짐" : L"꺼짐");
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
