// Shader.h
// HLSL 파일을 런타임에 컴파일한다.
//
// 빌드 시점에 .cso로 미리 컴파일하는 방법도 있지만, 학습 단계에서는
// 셰이더만 고치고 다시 실행하면 되는 런타임 컴파일이 훨씬 편하다.
// (배포 시에는 미리 컴파일하는 편이 시작 속도에 유리하다.)

#pragma once
#include "../Core/stdafx.h"

#include <d3dcommon.h>

class Shader
{
public:
	Shader() = default;
	~Shader() = default;

	// target 예: "vs_5_1", "ps_5_1"
	bool CompileFromFile(const std::wstring& filePath,
		const std::string& entryPoint,
		const std::string& target,
		const D3D_SHADER_MACRO* defines = nullptr);

	bool IsValid() const { return m_blob != nullptr; }

	D3D12_SHADER_BYTECODE GetBytecode() const;

private:
	ComPtr<ID3DBlob> m_blob;
	std::wstring m_filePath;
};
