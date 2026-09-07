// RootSignature.h
// 루트 시그니처 = 셰이더가 어떤 자원을(어떤 레지스터로) 받을지 적어 둔 명세.
// D3D11에는 없던 개념으로, PSO와 커맨드 리스트 양쪽에 설정해야 한다.
//
// 이 프레임워크는 디스크립터 테이블 대신 '루트 디스크립터(Root CBV)' 2개만 쓴다.
//   b0 = 오브젝트별 상수 (ObjectConstants)
//   b1 = 패스 공통 상수 (PassConstants)
// 텍스처가 없는 단계에서는 셰이더 가시 디스크립터 힙을 만들 필요조차 없어
// 코드가 크게 단순해진다. 텍스처를 붙일 때 디스크립터 테이블을 추가하면 된다.

#pragma once
#include "../Core/stdafx.h"

class RootSignature
{
public:
	RootSignature() = default;
	~RootSignature() = default;

	bool Initialize(ID3D12Device* device, const D3D12_ROOT_SIGNATURE_DESC& desc, const wchar_t* debugName);
	void Shutdown() { m_rootSignature.Reset(); }

	ID3D12RootSignature* Get() const { return m_rootSignature.Get(); }
	bool IsValid() const { return m_rootSignature != nullptr; }

	// 이 프레임워크의 기본 구성(b0 = 오브젝트, b1 = 패스)을 만들어 준다.
	bool InitializeDefault(ID3D12Device* device);

private:
	ComPtr<ID3D12RootSignature> m_rootSignature;
};
