// Window.h
// Win32 창 생성과 메시지 펌프만 담당한다.
// 렌더링 쪽과는 콜백으로만 연결되어 있어서(Window는 D3D를 전혀 모른다)
// 창 코드와 그래픽 코드를 독립적으로 수정할 수 있다.

#pragma once
#include "stdafx.h"

class Window
{
public:
	using ResizeCallback = std::function<void(UINT width, UINT height)>;
	// 창 프로시저가 받은 원시 메시지를 그대로 넘겨주는 콜백.
	// 입력 처리를 Window 밖(InputReader)에 두기 위한 통로다.
	using MessageCallback = std::function<void(UINT message, WPARAM wParam, LPARAM lParam)>;

	Window(HINSTANCE hInstance, const std::wstring& title, UINT width, UINT height);
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	bool Create();
	void Destroy();

	// 큐에 쌓인 메시지를 모두 처리한다. WM_QUIT을 받으면 false를 반환한다.
	bool ProcessMessages();

	HWND GetHandle() const { return m_hWnd; }
	UINT GetWidth() const { return m_width; }
	UINT GetHeight() const { return m_height; }
	bool IsMinimized() const { return m_minimized; }
	// 사용자가 창 테두리를 드래그하는 중인지. 드래그 중에는 리사이즈를 미룬다.
	bool IsResizing() const { return m_resizing; }
	bool IsPaused() const { return m_paused; }

	void SetTitle(const std::wstring& title);

	// 실제로 크기가 확정된 시점(드래그 종료/최대화/복원)에만 호출된다.
	void SetResizeCallback(ResizeCallback callback) { m_onResize = std::move(callback); }

	// 모든 메시지가 그대로 전달된다. 받는 쪽에서 필요한 것만 골라 쓴다.
	void SetMessageCallback(MessageCallback callback) { m_onMessage = std::move(callback); }

private:
	static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void NotifyResize(UINT width, UINT height);

	HINSTANCE m_hInstance = nullptr;
	HWND m_hWnd = nullptr;
	std::wstring m_title;
	UINT m_width = 0;
	UINT m_height = 0;

	bool m_minimized = false;
	bool m_maximized = false;
	bool m_resizing = false;
	bool m_paused = false;

	ResizeCallback m_onResize;
	MessageCallback m_onMessage;

	static const wchar_t* kClassName;
};
