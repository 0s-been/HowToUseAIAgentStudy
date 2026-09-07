#include "Timer.h"

Timer::Timer()
{
	INT64 countsPerSecond = 0;
	::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSecond));
	m_secondsPerCount = 1.0 / static_cast<double>(countsPerSecond);
}

float Timer::GetTotalTime() const
{
	// 멈춰 있는 동안은 시간이 흐르지 않아야 하므로 기준점을 m_stopTime으로 잡는다.
	// 어느 경우든 지금까지 누적된 일시정지 시간(m_pausedTime)은 빼 준다.
	const INT64 endTime = m_stopped ? m_stopTime : m_currTime;
	return static_cast<float>((endTime - m_pausedTime - m_baseTime) * m_secondsPerCount);
}

void Timer::Reset()
{
	INT64 currTime = 0;
	::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

	m_baseTime = currTime;
	m_prevTime = currTime;
	m_currTime = currTime;
	m_stopTime = 0;
	m_pausedTime = 0;
	m_stopped = false;

	m_frameCount = 0;
	m_elapsedForFps = 0.0;
}

void Timer::Start()
{
	if (!m_stopped)
	{
		return;
	}

	INT64 startTime = 0;
	::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));

	// 멈춰 있던 구간의 길이를 누적 일시정지 시간에 더한다.
	m_pausedTime += (startTime - m_stopTime);
	m_prevTime = startTime;
	m_stopTime = 0;
	m_stopped = false;
}

void Timer::Stop()
{
	if (m_stopped)
	{
		return;
	}

	::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_stopTime));
	m_stopped = true;
}

void Timer::Tick()
{
	if (m_stopped)
	{
		m_deltaTime = 0.0;
		return;
	}

	::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_currTime));
	m_deltaTime = (m_currTime - m_prevTime) * m_secondsPerCount;
	m_prevTime = m_currTime;

	// 절전 모드 진입/복귀나 다른 코어로의 스레드 이동 때문에
	// 음수가 나올 수 있다. 0으로 막아 둔다.
	if (m_deltaTime < 0.0)
	{
		m_deltaTime = 0.0;
	}

	UpdateFrameStats();
}

void Timer::UpdateFrameStats()
{
	++m_frameCount;
	m_elapsedForFps += m_deltaTime;

	if (m_elapsedForFps >= 1.0)
	{
		m_fps = static_cast<float>(m_frameCount / m_elapsedForFps);
		m_msPerFrame = (m_fps > 0.0f) ? (1000.0f / m_fps) : 0.0f;

		m_frameCount = 0;
		m_elapsedForFps = 0.0;
	}
}
