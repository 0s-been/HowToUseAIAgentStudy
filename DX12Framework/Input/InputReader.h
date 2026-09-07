// InputReader.h
// 키보드 / 마우스 입력을 모아 프레임 단위로 질의할 수 있게 해 준다.
//
// Window가 창 프로시저에서 받은 원시 메시지를 ProcessMessage로 흘려보내면,
// InputReader가 그것을 해석해 상태로 쌓아 둔다.
// 덕분에 Application은 "지금 W가 눌려 있는가?"처럼 게임 코드다운 형태로 물어볼 수 있다.
//
// [프레임당 호출 순서가 중요하다]
//   1) BeginFrame()          <- 이전 프레임 상태 보관, 마우스 이동량 0으로 초기화
//   2) Window::ProcessMessages()  <- 이 사이에 ProcessMessage가 여러 번 불린다
//   3) IsKeyDown / GetMouseDeltaX ... 질의
//
// BeginFrame을 메시지 펌프보다 '먼저' 부르는 것이 핵심이다.
// 그래야 WasKeyPressed(눌린 그 프레임만 true)가 정확히 한 프레임만 참이 된다.

#pragma once
#include "../Core/stdafx.h"

enum class MouseButton
{
	Left,
	Right,
	Middle,
	Count,
};

class InputReader
{
public:
	InputReader() = default;
	~InputReader();

	InputReader(const InputReader&) = delete;
	InputReader& operator=(const InputReader&) = delete;

	// 마우스 캡처에 창 핸들이 필요하다.
	void Initialize(HWND hWnd);

	// 창 프로시저가 받은 메시지를 그대로 넘긴다. 관심 없는 메시지는 무시한다.
	void ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);

	// 매 프레임 메시지 펌프 '직전'에 호출한다.
	void BeginFrame();

	// 포커스를 잃었을 때 눌린 키가 남지 않도록 전부 해제한다.
	void Reset();

	// --- 키보드 (virtualKey는 'W', VK_SHIFT 같은 가상 키 코드) ---
	bool IsKeyDown(int virtualKey) const;		// 눌려 있는 동안 계속 true
	bool WasKeyPressed(int virtualKey) const;	// 눌린 그 프레임만 true
	bool WasKeyReleased(int virtualKey) const;	// 떼어진 그 프레임만 true

	// --- 마우스 버튼 ---
	bool IsMouseDown(MouseButton button) const;
	bool WasMousePressed(MouseButton button) const;
	bool WasMouseReleased(MouseButton button) const;

	// 클라이언트 영역 기준 커서 위치
	int GetMouseX() const { return m_mouseX; }
	int GetMouseY() const { return m_mouseY; }

	// 이번 프레임 동안의 마우스 이동량(픽셀). 오른쪽/아래가 양수.
	float GetMouseDeltaX() const { return m_frameDeltaX; }
	float GetMouseDeltaY() const { return m_frameDeltaY; }

	// 휠 굴린 양. 한 칸이 1.0f.
	float GetMouseWheelDelta() const { return m_frameWheelDelta; }

private:
	static int ToIndex(MouseButton button) { return static_cast<int>(button); }

	void OnMouseButtonDown(MouseButton button);
	void OnMouseButtonUp(MouseButton button);
	void SetCursorHidden(bool hidden);

	static constexpr int kKeyCount = 256;
	static constexpr int kMouseButtonCount = static_cast<int>(MouseButton::Count);

	HWND m_hWnd = nullptr;

	bool m_keys[kKeyCount] = {};
	bool m_prevKeys[kKeyCount] = {};

	bool m_mouseButtons[kMouseButtonCount] = {};
	bool m_prevMouseButtons[kMouseButtonCount] = {};

	int m_mouseX = 0;
	int m_mouseY = 0;
	int m_lastMouseX = 0;
	int m_lastMouseY = 0;
	bool m_hasLastMousePosition = false;

	// 메시지 펌프 도중 누적되는 값. BeginFrame에서 0으로 초기화된다.
	float m_frameDeltaX = 0.0f;
	float m_frameDeltaY = 0.0f;
	float m_frameWheelDelta = 0.0f;

	// 우클릭 드래그 중 커서를 숨긴다. ShowCursor는 참조 카운트라 짝을 맞춰야 한다.
	bool m_cursorHidden = false;
	// 마우스 캡처 중인지. 캡처하면 창 밖으로 나가도 이동이 계속 들어온다.
	bool m_capturing = false;
};
