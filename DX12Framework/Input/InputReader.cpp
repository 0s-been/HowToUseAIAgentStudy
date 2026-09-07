#include "InputReader.h"

#include "../Core/Logger.h"

#include <windowsx.h>	// GET_X_LPARAM / GET_Y_LPARAM

InputReader::~InputReader()
{
	// 커서를 숨긴 채로 끝나면 다른 프로그램에서도 커서가 보이지 않는다.
	SetCursorHidden(false);
}

void InputReader::Initialize(HWND hWnd)
{
	m_hWnd = hWnd;
	Reset();
	LOG_INFO(L"입력 리더 초기화 완료 (WASD 이동 / 우클릭 드래그 회전)");
}

void InputReader::BeginFrame()
{
	// 이번 프레임의 '이전 상태'는 지난 프레임의 '현재 상태'다.
	std::memcpy(m_prevKeys, m_keys, sizeof(m_keys));
	std::memcpy(m_prevMouseButtons, m_mouseButtons, sizeof(m_mouseButtons));

	// 이동량은 누적값이므로 매 프레임 초기화한다.
	m_frameDeltaX = 0.0f;
	m_frameDeltaY = 0.0f;
	m_frameWheelDelta = 0.0f;
}

void InputReader::Reset()
{
	std::memset(m_keys, 0, sizeof(m_keys));
	std::memset(m_prevKeys, 0, sizeof(m_prevKeys));
	std::memset(m_mouseButtons, 0, sizeof(m_mouseButtons));
	std::memset(m_prevMouseButtons, 0, sizeof(m_prevMouseButtons));

	m_frameDeltaX = 0.0f;
	m_frameDeltaY = 0.0f;
	m_frameWheelDelta = 0.0f;
	m_hasLastMousePosition = false;

	if (m_capturing)
	{
		::ReleaseCapture();
		m_capturing = false;
	}
	SetCursorHidden(false);
}

void InputReader::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			// 키를 누르고 있으면 자동 반복 메시지가 계속 온다.
			// 이미 눌린 상태를 다시 true로 덮어도 결과는 같으므로 따로 거르지 않는다.
			// (WasKeyPressed는 이전 프레임 상태와 비교하므로 반복에 영향받지 않는다.)
			const int key = static_cast<int>(wParam);
			if (key >= 0 && key < kKeyCount)
			{
				m_keys[key] = true;
			}
			break;
		}

		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			const int key = static_cast<int>(wParam);
			if (key >= 0 && key < kKeyCount)
			{
				m_keys[key] = false;
			}
			break;
		}

		case WM_LBUTTONDOWN: OnMouseButtonDown(MouseButton::Left); break;
		case WM_RBUTTONDOWN: OnMouseButtonDown(MouseButton::Right); break;
		case WM_MBUTTONDOWN: OnMouseButtonDown(MouseButton::Middle); break;

		case WM_LBUTTONUP: OnMouseButtonUp(MouseButton::Left); break;
		case WM_RBUTTONUP: OnMouseButtonUp(MouseButton::Right); break;
		case WM_MBUTTONUP: OnMouseButtonUp(MouseButton::Middle); break;

		case WM_MOUSEMOVE:
		{
			const int x = GET_X_LPARAM(lParam);
			const int y = GET_Y_LPARAM(lParam);

			// 첫 메시지에서는 기준점이 없으므로 이동량을 만들지 않는다.
			// 이 처리를 빼면 창에 처음 마우스를 올릴 때 화면이 확 튄다.
			if (m_hasLastMousePosition)
			{
				m_frameDeltaX += static_cast<float>(x - m_lastMouseX);
				m_frameDeltaY += static_cast<float>(y - m_lastMouseY);
			}

			m_mouseX = x;
			m_mouseY = y;
			m_lastMouseX = x;
			m_lastMouseY = y;
			m_hasLastMousePosition = true;
			break;
		}

		case WM_MOUSEWHEEL:
			m_frameWheelDelta += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
			break;

		case WM_CAPTURECHANGED:
			// 다른 창이 캡처를 가져갔다. 우리가 잡고 있다고 착각하면 안 된다.
			if (m_capturing)
			{
				m_capturing = false;
				m_mouseButtons[ToIndex(MouseButton::Right)] = false;
				SetCursorHidden(false);
			}
			break;

		case WM_KILLFOCUS:
			// 포커스를 잃으면 키를 떼는 메시지가 오지 않는다.
			// 초기화하지 않으면 다시 돌아왔을 때 계속 눌린 것처럼 동작한다.
			Reset();
			break;

		default:
			break;
	}
}

void InputReader::OnMouseButtonDown(MouseButton button)
{
	m_mouseButtons[ToIndex(button)] = true;

	if (button == MouseButton::Right && m_hWnd != nullptr && !m_capturing)
	{
		// 캡처해 두면 드래그가 창 밖으로 나가도 이동량이 계속 들어온다.
		::SetCapture(m_hWnd);
		m_capturing = true;
		SetCursorHidden(true);

		// 드래그를 시작하는 순간 기준점을 현재 위치로 맞춘다.
		// 그러지 않으면 직전에 마우스가 멀리 있었을 때 시야가 확 돌아간다.
		POINT cursor = {};
		if (::GetCursorPos(&cursor) && ::ScreenToClient(m_hWnd, &cursor))
		{
			m_lastMouseX = cursor.x;
			m_lastMouseY = cursor.y;
			m_mouseX = cursor.x;
			m_mouseY = cursor.y;
			m_hasLastMousePosition = true;
		}
	}
}

void InputReader::OnMouseButtonUp(MouseButton button)
{
	m_mouseButtons[ToIndex(button)] = false;

	if (button == MouseButton::Right && m_capturing)
	{
		::ReleaseCapture();
		m_capturing = false;
		SetCursorHidden(false);
	}
}

void InputReader::SetCursorHidden(bool hidden)
{
	if (hidden == m_cursorHidden)
	{
		return;
	}

	// ShowCursor는 내부 카운터를 올리고 내린다. 호출 횟수의 짝이 맞아야 한다.
	::ShowCursor(hidden ? FALSE : TRUE);
	m_cursorHidden = hidden;
}

bool InputReader::IsKeyDown(int virtualKey) const
{
	if (virtualKey < 0 || virtualKey >= kKeyCount)
	{
		return false;
	}
	return m_keys[virtualKey];
}

bool InputReader::WasKeyPressed(int virtualKey) const
{
	if (virtualKey < 0 || virtualKey >= kKeyCount)
	{
		return false;
	}
	return m_keys[virtualKey] && !m_prevKeys[virtualKey];
}

bool InputReader::WasKeyReleased(int virtualKey) const
{
	if (virtualKey < 0 || virtualKey >= kKeyCount)
	{
		return false;
	}
	return !m_keys[virtualKey] && m_prevKeys[virtualKey];
}

bool InputReader::IsMouseDown(MouseButton button) const
{
	return m_mouseButtons[ToIndex(button)];
}

bool InputReader::WasMousePressed(MouseButton button) const
{
	const int i = ToIndex(button);
	return m_mouseButtons[i] && !m_prevMouseButtons[i];
}

bool InputReader::WasMouseReleased(MouseButton button) const
{
	const int i = ToIndex(button);
	return !m_mouseButtons[i] && m_prevMouseButtons[i];
}
