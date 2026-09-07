// Timer.h
// QueryPerformanceCounter 기반 고해상도 타이머.
// 프레임 간 경과 시간(DeltaTime)과 시작 이후 누적 시간(TotalTime)을 제공한다.
// 일시정지(Stop) 구간은 TotalTime에서 제외된다.

#pragma once
#include "stdafx.h"

class Timer
{
public:
	Timer();

	// 초 단위.
	float GetDeltaTime() const { return static_cast<float>(m_deltaTime); }
	float GetTotalTime() const;

	// 직전 1초 동안 측정된 평균 FPS.
	float GetFps() const { return m_fps; }
	// 프레임 1장당 밀리초.
	float GetMsPerFrame() const { return m_msPerFrame; }

	void Reset();	// 루프 시작 직전에 한 번 호출한다.
	void Start();	// 일시정지 해제.
	void Stop();	// 일시정지.
	void Tick();	// 매 프레임 한 번 호출한다.

private:
	void UpdateFrameStats();

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
