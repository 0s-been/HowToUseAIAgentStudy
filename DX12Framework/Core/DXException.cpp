#include "DXException.h"

#include <comdef.h>

DxException::DxException(HRESULT hr, const std::wstring& expression, const std::wstring& file, int line)
	: m_errorCode(hr)
	, m_expression(expression)
	, m_file(file)
	, m_line(line)
{
}

std::wstring DxException::ToString() const
{
	// _com_error가 HRESULT를 사람이 읽을 수 있는 메시지로 변환해 준다.
	const _com_error err(m_errorCode);

	wchar_t code[32] = {};
	swprintf_s(code, L"0x%08lX", static_cast<unsigned long>(m_errorCode));

	return m_file + L"(" + std::to_wstring(m_line) + L"): " + m_expression +
		L" -> " + err.ErrorMessage() + L" (HRESULT = " + code + L")";
}
