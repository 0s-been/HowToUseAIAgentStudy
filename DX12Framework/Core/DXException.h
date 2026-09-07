// DXException.h
// HRESULT 실패를 예외로 바꿔주는 얇은 래퍼.
// D3D12는 거의 모든 API가 HRESULT를 돌려주기 때문에, 호출부마다 if 문을 쓰는 대신
// ThrowIfFailed로 감싸고 최상위(Application::Initialize)에서 한 번만 처리한다.

#pragma once
#include "stdafx.h"

class DxException
{
public:
	DxException(HRESULT hr, const std::wstring& expression, const std::wstring& file, int line);

	// "파일(줄): 실패한 식 -> HRESULT 설명" 형태의 문자열을 만든다.
	std::wstring ToString() const;

	HRESULT GetErrorCode() const { return m_errorCode; }

private:
	HRESULT m_errorCode;
	std::wstring m_expression;
	std::wstring m_file;
	int m_line;
};

// L#x 는 인자로 넘어온 식을 그대로 와이드 문자열 리터럴로 만든다.
// do/while(0)로 감싸 if 문 뒤에 붙어도 안전하게 동작하게 한다.
#define ThrowIfFailed(x)                                                       \
	do                                                                         \
	{                                                                          \
		HRESULT hr__ = (x);                                                    \
		if (FAILED(hr__))                                                      \
		{                                                                      \
			throw DxException(hr__, L#x, __FILEW__, __LINE__);                 \
		}                                                                      \
	} while (0)
