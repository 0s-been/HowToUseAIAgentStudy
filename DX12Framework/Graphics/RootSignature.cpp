#include "RootSignature.h"

#include "../Core/Logger.h"

bool RootSignature::Initialize(ID3D12Device* device, const D3D12_ROOT_SIGNATURE_DESC& desc, const wchar_t* debugName)
{
	ComPtr<ID3DBlob> serialized;
	ComPtr<ID3DBlob> errors;

	const HRESULT hr = ::D3D12SerializeRootSignature(
		&desc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors);

	if (errors != nullptr && errors->GetBufferSize() > 0)
	{
		LOG_ERROR(L"루트 시그니처 직렬화 오류: %S", static_cast<const char*>(errors->GetBufferPointer()));
	}

	if (FAILED(hr))
	{
		LOG_ERROR(L"D3D12SerializeRootSignature 실패. HRESULT = 0x%08lX", static_cast<unsigned long>(hr));
		return false;
	}

	const HRESULT createHr = device->CreateRootSignature(
		0,
		serialized->GetBufferPointer(),
		serialized->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature));

	if (FAILED(createHr))
	{
		LOG_ERROR(L"CreateRootSignature 실패. HRESULT = 0x%08lX", static_cast<unsigned long>(createHr));
		return false;
	}

	if (debugName != nullptr)
	{
		m_rootSignature->SetName(debugName);
	}

	LOG_INFO(L"루트 시그니처 생성 완료: %s (루트 파라미터 %u개)",
		(debugName != nullptr) ? debugName : L"(이름 없음)", desc.NumParameters);
	return true;
}

bool RootSignature::InitializeDefault(ID3D12Device* device)
{
	D3D12_ROOT_PARAMETER params[2] = {};

	// b0: 오브젝트별 상수. 정점 셰이더에서만 쓰므로 가시성을 좁혀 준다.
	// (가시성을 좁히면 드라이버가 더 나은 최적화를 할 수 있다.)
	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[0].Descriptor.ShaderRegister = 0;
	params[0].Descriptor.RegisterSpace = 0;
	params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// b1: 패스 공통 상수. 정점(변환)과 픽셀(조명) 양쪽에서 쓴다.
	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[1].Descriptor.ShaderRegister = 1;
	params[1].Descriptor.RegisterSpace = 0;
	params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = _countof(params);
	desc.pParameters = params;
	desc.NumStaticSamplers = 0;
	desc.pStaticSamplers = nullptr;
	// 입력 어셈블러를 쓰므로 이 플래그가 필요하다. 빠뜨리면 정점 버퍼가 무시된다.
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	return Initialize(device, desc, L"DefaultRootSignature");
}
