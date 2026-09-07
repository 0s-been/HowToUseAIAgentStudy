#include "Window.h"

#include "Logger.h"

const wchar_t* Window::kClassName = L"DX12FrameworkWindowClass";

Window::Window(HINSTANCE hInstance, const std::wstring& title, UINT width, UINT height)
	: m_hInstance(hInstance)
	, m_title(title)
	, m_width(width)
	, m_height(height)
{
}

Window::~Window()
{
	Destroy();
}

bool Window::Create()
{
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;	// 크기가 바뀌면 전체를 다시 그린다.
	wc.lpfnWndProc = &Window::WndProcStatic;
	wc.hInstance = m_hInstance;
	wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;	// D3D가 전체를 덮으므로 GDI 배경칠은 하지 않는다.
	wc.lpszClassName = kClassName;

	if (::RegisterClassExW(&wc) == 0)
	{
		// 이미 등록된 클래스라면 계속 진행해도 된다.
		if (::GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
		{
			LOG_ERROR(L"윈도우 클래스 등록 실패. GetLastError = %lu", ::GetLastError());
			return false;
		}
	}

	// 요청한 크기는 '클라이언트 영역' 기준이다.
	// 테두리와 타이틀바를 포함한 전체 창 크기로 보정한다.
	const DWORD style = WS_OVERLAPPEDWINDOW;
	RECT rect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
	::AdjustWindowRect(&rect, style, FALSE);

	const int windowWidth = rect.right - rect.left;
	const int windowHeight = rect.bottom - rect.top;

	m_hWnd = ::CreateWindowExW(
		0,
		kClassName,
		m_title.c_str(),
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowWidth, windowHeight,
		nullptr, nullptr,
		m_hInstance,
		this);	// WM_NCCREATE에서 this 포인터를 돌려받는다.

	if (m_hWnd == nullptr)
	{
		LOG_ERROR(L"윈도우 생성 실패. GetLastError = %lu", ::GetLastError());
		return false;
	}

	::ShowWindow(m_hWnd, SW_SHOW);
	::UpdateWindow(m_hWnd);

	LOG_INFO(L"윈도우 생성 완료. 클라이언트 영역 %u x %u", m_width, m_height);
	return true;
}

void Window::Destroy()
{
	if (m_hWnd != nullptr)
	{
		::DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}
}

bool Window::ProcessMessages()
{
	MSG msg = {};
	while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return false;
		}

		::TranslateMessage(&msg);
		::DispatchMessageW(&msg);
	}
	return true;
}

void Window::SetTitle(const std::wstring& title)
{
	m_title = title;
	if (m_hWnd != nullptr)
	{
		::SetWindowTextW(m_hWnd, m_title.c_str());
	}
}

void Window::NotifyResize(UINT width, UINT height)
{
	// 최소화 등으로 0이 되는 경우가 있다. 이때 스왑체인을 재생성하면 실패한다.
	if (width == 0 || height == 0)
	{
		return;
	}

	m_width = width;
	m_height = height;

	if (m_onResize)
	{
		m_onResize(width, height);
	}
}

LRESULT CALLBACK Window::WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// 창이 만들어지는 시점에 this를 GWLP_USERDATA에 저장해 두고,
	// 이후 메시지에서 꺼내 멤버 함수로 넘긴다.
	if (message == WM_NCCREATE)
	{
		const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
		auto* window = static_cast<Window*>(create->lpCreateParams);
		::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
	}

	auto* window = reinterpret_cast<Window*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	if (window != nullptr)
	{
		return window->WndProc(hWnd, message, wParam, lParam);
	}

	return ::DefWindowProcW(hWnd, message, wParam, lParam);
}

LRESULT Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// 창 자신의 처리보다 먼저 넘긴다. 입력 상태가 한 프레임 늦게 반영되는 일이 없다.
	if (m_onMessage)
	{
		m_onMessage(message, wParam, lParam);
	}

	switch (message)
	{
		case WM_ACTIVATE:
			// 창이 비활성화되면 시뮬레이션을 멈춘다.
			m_paused = (LOWORD(wParam) == WA_INACTIVE);
			return 0;

		case WM_SIZE:
		{
			const UINT width = LOWORD(lParam);
			const UINT height = HIWORD(lParam);

			if (wParam == SIZE_MINIMIZED)
			{
				m_paused = true;
				m_minimized = true;
				m_maximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				m_paused = false;
				m_minimized = false;
				m_maximized = true;
				NotifyResize(width, height);
			}
			else if (wParam == SIZE_RESTORED)
			{
				if (m_minimized)
				{
					// 최소화 -> 복원
					m_paused = false;
					m_minimized = false;
					NotifyResize(width, height);
				}
				else if (m_maximized)
				{
					// 최대화 -> 복원
					m_paused = false;
					m_maximized = false;
					NotifyResize(width, height);
				}
				else if (!m_resizing)
				{
					// 테두리 드래그가 아닌 경로(예: SetWindowPos)로 크기가 바뀐 경우.
					// 드래그 중이라면 WM_EXITSIZEMOVE에서 한 번만 처리한다.
					NotifyResize(width, height);
				}
			}
			return 0;
		}

		case WM_ENTERSIZEMOVE:
			// 테두리 드래그 시작. 드래그 도중에는 WM_SIZE가 수십 번 오므로
			// 스왑체인 재생성을 미뤄 두고 드래그가 끝날 때 한 번만 한다.
			m_paused = true;
			m_resizing = true;
			return 0;

		case WM_EXITSIZEMOVE:
		{
			m_paused = false;
			m_resizing = false;

			RECT client = {};
			::GetClientRect(hWnd, &client);
			NotifyResize(static_cast<UINT>(client.right - client.left),
				static_cast<UINT>(client.bottom - client.top));
			return 0;
		}

		case WM_GETMINMAXINFO:
			// 창이 너무 작아져서 스왑체인이 깨지는 것을 막는다.
			reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize.x = 320;
			reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize.y = 240;
			return 0;

		case WM_KEYDOWN:
			// 창을 닫는 것은 창의 책임이다. 나머지 키는 InputReader가 받아 처리한다.
			if (wParam == VK_ESCAPE)
			{
				::PostQuitMessage(0);
				return 0;
			}
			break;

		case WM_MENUCHAR:
			// Alt + Enter 등에서 나는 비프음을 막는다.
			return MAKELRESULT(0, MNC_CLOSE);

		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;

		default:
			break;
	}

	return ::DefWindowProcW(hWnd, message, wParam, lParam);
}
