// Logger.h
// 프레임워크 각 단계(디바이스 생성 / 스왑체인 / 셰이더 컴파일 ...)의 진행 상황을 남기는 로거.
//
// 출력 대상 3곳:
//   1) Visual Studio 출력 창 (OutputDebugStringW)
//   2) 콘솔 창 (디버그 빌드에서 AllocConsole로 띄운다)
//   3) 실행 파일 폴더의 로그 파일 (UTF-8, BOM 포함 -> 메모장에서 한글이 깨지지 않는다)

#pragma once
#include "stdafx.h"

#include <fstream>
#include <mutex>

enum class LogLevel
{
	Trace,
	Info,
	Warning,
	Error,
};

class Logger
{
public:
	static Logger& Get();

	// fileName이 비어 있으면 파일 출력을 하지 않는다.
	void Initialize(const std::wstring& fileName = L"DX12Framework.log", bool allocConsole = true);
	void Shutdown();

	// 이 레벨보다 낮은 로그는 버린다.
	void SetMinLevel(LogLevel level) { m_minLevel = level; }

	void Write(LogLevel level, const wchar_t* file, int line, const wchar_t* format, ...);

private:
	Logger() = default;
	~Logger();

	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

	static const wchar_t* LevelToString(LogLevel level);
	static const wchar_t* FileNameOnly(const wchar_t* path);
	static std::wstring TimeStamp();

	void WriteToConsole(const std::wstring& text);
	void WriteToFile(const std::wstring& text);

	std::ofstream m_file;
	std::mutex m_mutex;
	LogLevel m_minLevel = LogLevel::Trace;
	bool m_consoleAllocated = false;
	bool m_initialized = false;
};

// 서식 문자열까지 __VA_ARGS__에 포함시킨다.
// 이렇게 하면 __VA_ARGS__가 절대 비지 않으므로, MSVC의 기존 전처리기와
// 표준 준수 전처리기(/Zc:preprocessor) 양쪽에서 모두 동작한다.
#define LOG_TRACE(...) Logger::Get().Write(LogLevel::Trace, __FILEW__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  Logger::Get().Write(LogLevel::Info, __FILEW__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  Logger::Get().Write(LogLevel::Warning, __FILEW__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) Logger::Get().Write(LogLevel::Error, __FILEW__, __LINE__, __VA_ARGS__)
