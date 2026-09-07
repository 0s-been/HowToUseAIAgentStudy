// PipelineState.h
// PSO(Pipeline State Object) 래퍼.
//
// D3D11에서 따로따로 설정하던 래스터라이저/블렌드/깊이스텐실/셰이더/입력 레이아웃이
// D3D12에서는 하나의 불변 객체로 묶인다. 미리 만들어 두고 그리기 직전에 통째로 교체한다.
// 상태 조합이 바뀔 때마다 새 PSO가 필요하므로, 보통 초기화 시점에 몰아서 만든다.

#pragma once
#include "../Core/stdafx.h"

class PipelineState
{
public:
	PipelineState() = default;
	~PipelineState() = default;

	// 자주 쓰는 값이 채워진 설명자를 돌려준다.
	// 호출부에서 셰이더/루트 시그니처/입력 레이아웃/포맷만 채우면 된다.
	static D3D12_GRAPHICS_PIPELINE_STATE_DESC MakeDefaultDesc();

	bool Initialize(ID3D12Device* device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const wchar_t* debugName);
	void Shutdown() { m_pipelineState.Reset(); }

	ID3D12PipelineState* Get() const { return m_pipelineState.Get(); }
	bool IsValid() const { return m_pipelineState != nullptr; }

private:
	ComPtr<ID3D12PipelineState> m_pipelineState;
};
