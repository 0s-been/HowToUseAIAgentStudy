// Timer.h
// QueryPerformanceCounter 기반 고해상도 타이머.
// 프레임 간 경과 시간(DeltaTime)과 시작 이후 누적 시간(TotalTime)을 제공한다.
// 일시정지(Stop) 구간은 TotalTime에서 제외된다.
//
// [DeltaTime 상한을 두는 이유]
// 디버거로 멈췄다 재개하거나, 알트탭, 드라이버 hitch 등으로 한 프레임이 아주 오래
// 걸리면 그다음 Tick()의 델타가 순간적으로 매우 커진다. 이동/회전을 deltaTime에
// 비례시키는 코드(Application::Update)는 그 큰 델타를 그대로 받아 카메라나 오브젝트를
// 한 프레임 만에 크게 점프시키는데, 이게 "빠르게 움직였더니 화면이 번진다"처럼 보이는
// 흔한 원인이다("spiral of death"라고도 부른다). GetDeltaTime()이 돌려주는 값에는
// 상한을 걸어 두고, FPS 통계에는 실제(clamp 되지 않은) 델타를 그대로 써서 진짜 프레임률은
// 왜곡 없이 보이게 한다.

#pragma once
#include "stdafx.h"

class Timer
{
public:
	Timer();

	// 초 단위.
	float GetDeltaTime() const { return static_cast<float>(m_deltaTime); }
	float GetTotalTime() const;

	// 직전 1초 동안 측정된 평균 FPS. (clamp 되지 않은 실제 값)
	float GetFps() const { return m_fps; }
	// 프레임 1장당 밀리초. (clamp 되지 않은 실제 값)
	float GetMsPerFrame() const { return m_msPerFrame; }

	void Reset();	// 루프 시작 직전에 한 번 호출한다.
	void Start();	// 일시정지 해제.
	void Stop();	// 일시정지.
	void Tick();	// 매 프레임 한 번 호출한다.

private:
	void UpdateFrameStats(double rawDeltaTime);

	// 한 프레임의 델타가 이 값을 넘으면 여기서 자른다. 100ms면 초당 10프레임보다
	// 느려진 경우인데, 그 이하로 떨어졌다면 어차피 정상 플레이가 아니라 hitch로 본다.
	static constexpr double kMaxDeltaTime = 0.1;

	double m_secondsPerCount = 0.0;
	double m_deltaTime = 0.0;

	INT64 m_baseTime = 0;
	INT64 m_pausedTime = 0;
	INT64 m_stopTime = 0;
	INT64 m_prevTime = 0;
	INT64 m_currTime = 0;

	bool m_stopped = false;

	// FPS 집계용
	int m_frameCount = 0;
	double m_elapsedForFps = 0.0;
	float m_fps = 0.0f;
	float m_msPerFrame = 0.0f;
};
