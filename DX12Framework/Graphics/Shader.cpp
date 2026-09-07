#include "Shader.h"

#include "../Core/Logger.h"

#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

bool Shader::CompileFromFile(const std::wstring& filePath,
	const std::string& entryPoint,
	const std::string& target,
	const D3D_SHADER_MACRO* defines)
{
	m_filePath = filePath;

	UINT flags = 0;
#if defined(_DEBUG)
	// 디버그 정보를 남기고 최적화를 끄면 PIX/RenderDoc에서 셰이더를 단계 실행할 수 있다.
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> errors;
	const HRESULT hr = ::D3DCompileFromFile(
		filePath.c_str(),
		defines,
		// 셰이더 안에서 #include를 쓸 수 있게 해 준다.
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint.c_str(),
		target.c_str(),
		flags,
		0,
		&m_blob,
		&errors);

	// 컴파일에 성공해도 경고가 담겨 올 수 있으므로 항상 확인한다.
	if (errors != nullptr && errors->GetBufferSize() > 0)
	{
		const char* text = static_cast<const char*>(errors->GetBufferPointer());
		if (FAILED(hr))
		{
			LOG_ERROR(L"셰이더 컴파일 오류 [%s / %S]:\n%S", filePath.c_str(), entryPoint.c_str(), text);
		}
		else
		{
			LOG_WARN(L"셰이더 컴파일 경고 [%s / %S]:\n%S", filePath.c_str(), entryPoint.c_str(), text);
		}
	}

	if (FAILED(hr))
	{
		// 파일 자체를 못 찾은 경우가 가장 흔하다. 작업 디렉터리를 의심해야 한다.
		if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
		{
			LOG_ERROR(L"셰이더 파일을 찾을 수 없다: %s (프로젝트 속성의 작업 디렉터리를 확인할 것)",
				filePath.c_str());
		}
		else
		{
			LOG_ERROR(L"셰이더 컴파일 실패: %s (HRESULT = 0x%08lX)",
				filePath.c_str(), static_cast<unsigned long>(hr));
		}
		m_blob.Reset();
		return false;
	}

	LOG_INFO(L"셰이더 컴파일 완료: %s / %S / %S (%zu 바이트)",
		filePath.c_str(), entryPoint.c_str(), target.c_str(), m_blob->GetBufferSize());
	return true;
}

D3D12_SHADER_BYTECODE Shader::GetBytecode() const
{
	D3D12_SHADER_BYTECODE bytecode = {};
	if (m_blob != nullptr)
	{
		bytecode.pShaderBytecode = m_blob->GetBufferPointer();
		bytecode.BytecodeLength = m_blob->GetBufferSize();
	}
	return bytecode;
}
