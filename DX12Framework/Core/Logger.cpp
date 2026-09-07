#include "Logger.h"

#include <cstdarg>

namespace
{
	// 와이드 문자열을 UTF-8 바이트로 변환한다. (파일 출력용)
	std::string ToUtf8(const std::wstring& text)
	{
		if (text.empty())
		{
			return {};
		}

		const int byteCount = ::WideCharToMultiByte(
			CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
		if (byteCount <= 0)
		{
			return {};
		}

		std::string result(static_cast<size_t>(byteCount), '\0');
		::WideCharToMultiByte(
			CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), byteCount, nullptr, nullptr);
		return result;
	}
}

Logger& Logger::Get()
{
	// 함수 지역 static이라 최초 호출 시점에 생성되고 프로그램 종료 시 자동 소멸한다.
	static Logger instance;
	return instance;
}

Logger::~Logger()
{
	Shutdown();
}

void Logger::Initialize(const std::wstring& fileName, bool allocConsole)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_initialized)
	{
		return;
	}

	if (allocConsole && ::AllocConsole())
	{
		m_consoleAllocated = true;
		::SetConsoleTitleW(L"DX12Framework Log");
	}

	if (!fileName.empty())
	{
		m_file.open(fileName.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
		if (m_file.is_open())
		{
			// UTF-8 BOM. 메모장 등에서 한글이 깨지지 않도록 한다.
			const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
			m_file.write(reinterpret_cast<const char*>(bom), sizeof(bom));
		}
	}

	m_initialized = true;
}

void Logger::Shutdown()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (!m_initialized)
	{
		return;
	}

	if (m_file.is_open())
	{
		m_file.flush();
		m_file.close();
	}

	if (m_consoleAllocated)
	{
		::FreeConsole();
		m_consoleAllocated = false;
	}

	m_initialized = false;
}

const wchar_t* Logger::LevelToString(LogLevel level)
{
	switch (level)
	{
		case LogLevel::Trace:   return L"TRACE";
		case LogLevel::Info:    return L"INFO ";
		case LogLevel::Warning: return L"WARN ";
		case LogLevel::Error:   return L"ERROR";
	}
	return L"?????";
}

const wchar_t* Logger::FileNameOnly(const wchar_t* path)
{
	if (path == nullptr)
	{
		return L"";
	}

	const wchar_t* name = path;
	for (const wchar_t* p = path; *p != L'\0'; ++p)
	{
		if (*p == L'\\' || *p == L'/')
		{
			name = p + 1;
		}
	}
	return name;
}

std::wstring Logger::TimeStamp()
{
	SYSTEMTIME st = {};
	::GetLocalTime(&st);

	wchar_t buffer[32] = {};
	swprintf_s(buffer, L"%02d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return buffer;
}

void Logger::WriteToConsole(const std::wstring& text)
{
	// WriteConsoleW를 쓰면 콘솔 코드 페이지와 무관하게 유니코드가 그대로 출력된다.
	const HANDLE console = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (console == nullptr || console == INVALID_HANDLE_VALUE)
	{
		return;
	}

	DWORD written = 0;
	::WriteConsoleW(console, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
}

void Logger::WriteToFile(const std::wstring& text)
{
	if (!m_file.is_open())
	{
		return;
	}

	const std::string utf8 = ToUtf8(text);
	m_file.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
	m_file.flush();	// 크래시로 죽어도 직전까지의 로그는 남도록 매번 flush한다.
}

void Logger::Write(LogLevel level, const wchar_t* file, int line, const wchar_t* format, ...)
{
	if (level < m_minLevel)
	{
		return;
	}

	wchar_t message[2048] = {};
	va_list args;
	va_start(args, format);
	_vsnwprintf_s(message, _countof(message), _TRUNCATE, format, args);
	va_end(args);

	wchar_t fullLine[2560] = {};
	swprintf_s(fullLine, L"[%s][%s] %s  (%s:%d)\n",
		TimeStamp().c_str(), LevelToString(level), message, FileNameOnly(file), line);

	std::lock_guard<std::mutex> lock(m_mutex);

	::OutputDebugStringW(fullLine);
	WriteToConsole(fullLine);
	WriteToFile(fullLine);
}
