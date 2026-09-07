#include "PipelineState.h"

#include "../Core/Logger.h"
#include "D3D12Helpers.h"

D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineState::MakeDefaultDesc()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.RasterizerState = DX::DefaultRasterizerDesc();
	desc.BlendState = DX::OpaqueBlendDesc();
	desc.DepthStencilState = DX::DefaultDepthStencilDesc();
	// 모든 샘플을 렌더링한다. 0으로 두면 아무것도 그려지지 않는 흔한 실수가 된다.
	desc.SampleMask = UINT_MAX;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	return desc;
}

bool PipelineState::Initialize(ID3D12Device* device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const wchar_t* debugName)
{
	const HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipelineState));
	if (FAILED(hr))
	{
		LOG_ERROR(L"파이프라인 상태 생성 실패: %s (HRESULT = 0x%08lX)",
			(debugName != nullptr) ? debugName : L"(이름 없음)", static_cast<unsigned long>(hr));
		return false;
	}

	if (debugName != nullptr)
	{
		m_pipelineState->SetName(debugName);
	}

	LOG_INFO(L"파이프라인 상태 생성 완료: %s", (debugName != nullptr) ? debugName : L"(이름 없음)");
	return true;
}
