// D3D12Device.h
// DXGI 팩토리 / 어댑터 선택 / ID3D12Device 생성까지를 담당한다.
// D3D11의 D3D11CreateDeviceAndSwapChain과 달리 D3D12는 디바이스, 큐, 스왑체인이
// 모두 분리되어 있으므로 이 클래스는 "디바이스"만 책임진다.

#pragma once
#include "../Core/stdafx.h"

class D3D12Device
{
public:
	D3D12Device() = default;
	~D3D12Device();

	D3D12Device(const D3D12Device&) = delete;
	D3D12Device& operator=(const D3D12Device&) = delete;

	// enableDebugLayer는 디버그 빌드에서만 의미가 있다.
	// 디버그 레이어는 반드시 디바이스 생성 '전에' 켜야 적용된다.
	bool Initialize(bool enableDebugLayer);
	void Shutdown();

	ID3D12Device* Get() const { return m_device.Get(); }
	IDXGIFactory4* GetFactory() const { return m_factory.Get(); }

	// 디스크립터 한 칸의 바이트 크기. 하드웨어마다 다르므로 반드시 런타임에 조회해야 한다.
	UINT GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

	// 가변 주사율(티어링) 지원 여부. 스왑체인 생성/Present 플래그에 쓰인다.
	bool IsTearingSupported() const { return m_tearingSupported; }

	const std::wstring& GetAdapterName() const { return m_adapterName; }
	D3D_FEATURE_LEVEL GetFeatureLevel() const { return m_featureLevel; }

	// 종료 시 해제되지 않은 D3D 객체를 출력 창에 보고한다. (디버그 빌드 전용)
	static void ReportLiveObjects();

private:
	static void EnableDebugLayer();
	bool CreateFactory(bool enableDebugLayer);
	ComPtr<IDXGIAdapter1> SelectAdapter();
	void ConfigureInfoQueue() const;
	void CacheDescriptorSizes();
	void QueryFeatureLevel();
	void CheckTearingSupport();

	ComPtr<IDXGIFactory4> m_factory;
	ComPtr<IDXGIAdapter1> m_adapter;
	ComPtr<ID3D12Device> m_device;

	std::wstring m_adapterName;
	D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;
	bool m_tearingSupported = false;

	// D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES 개수만큼 캐시해 둔다.
	UINT m_descriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
};
