#include "D3D12Device.h"

#include "../Core/DXException.h"
#include "../Core/Logger.h"

#include <dxgidebug.h>

D3D12Device::~D3D12Device()
{
	Shutdown();
}

bool D3D12Device::Initialize(bool enableDebugLayer)
{
	try
	{
		// 순서가 중요하다. 디버그 레이어는 디바이스를 만들기 전에 켜야 한다.
		if (enableDebugLayer)
		{
			EnableDebugLayer();
		}

		if (!CreateFactory(enableDebugLayer))
		{
			return false;
		}

		m_adapter = SelectAdapter();
		if (m_adapter == nullptr)
		{
			LOG_ERROR(L"D3D12를 지원하는 그래픽 어댑터를 찾지 못했다.");
			return false;
		}

		// 최소 기능 레벨 11_0으로 생성한다. 실제 지원 상한은 뒤에서 따로 조회한다.
		ThrowIfFailed(::D3D12CreateDevice(
			m_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
		m_device->SetName(L"MainDevice");

		if (enableDebugLayer)
		{
			ConfigureInfoQueue();
		}

		CacheDescriptorSizes();
		QueryFeatureLevel();
		CheckTearingSupport();

		LOG_INFO(L"D3D12 디바이스 생성 완료");
		LOG_INFO(L"  어댑터      : %s", m_adapterName.c_str());
		LOG_INFO(L"  기능 레벨   : %u.%u",
			(static_cast<unsigned>(m_featureLevel) >> 12) & 0xF,
			(static_cast<unsigned>(m_featureLevel) >> 8) & 0xF);
		LOG_INFO(L"  디스크립터 크기 : RTV %u / DSV %u / CBV_SRV_UAV %u / SAMPLER %u",
			m_descriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV],
			m_descriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_DSV],
			m_descriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV],
			m_descriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]);
		LOG_INFO(L"  티어링 지원 : %s", m_tearingSupported ? L"예" : L"아니오");
		return true;
	}
	catch (const DxException& e)
	{
		LOG_ERROR(L"디바이스 초기화 실패: %s", e.ToString().c_str());
		return false;
	}
}

void D3D12Device::Shutdown()
{
	m_device.Reset();
	m_adapter.Reset();
	m_factory.Reset();
}

void D3D12Device::EnableDebugLayer()
{
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(::D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		LOG_INFO(L"D3D12 디버그 레이어 활성화");

		// GPU 기반 검증은 디스크립터/리소스 상태 오류를 훨씬 정확히 잡아주지만
		// 매우 느리다. 필요할 때만 주석을 풀어 쓴다.
		// ComPtr<ID3D12Debug1> debugController1;
		// if (SUCCEEDED(debugController.As(&debugController1)))
		// {
		//     debugController1->SetEnableGPUBasedValidation(TRUE);
		// }
	}
	else
	{
		LOG_WARN(L"디버그 레이어를 활성화하지 못했다. (그래픽 도구 기능이 설치되지 않았을 수 있다)");
	}
#endif
}

bool D3D12Device::CreateFactory(bool enableDebugLayer)
{
	UINT flags = 0;
#if defined(_DEBUG)
	if (enableDebugLayer)
	{
		flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#else
	UNREFERENCED_PARAMETER(enableDebugLayer);
#endif

	const HRESULT hr = ::CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory));
	if (FAILED(hr))
	{
		LOG_ERROR(L"CreateDXGIFactory2 실패. HRESULT = 0x%08lX", static_cast<unsigned long>(hr));
		return false;
	}
	return true;
}

ComPtr<IDXGIAdapter1> D3D12Device::SelectAdapter()
{
	// IDXGIFactory6를 지원하면 드라이버가 정렬해 주는 '고성능 우선' 순서를 그대로 쓴다.
	// 노트북처럼 내장/외장 GPU가 함께 있는 환경에서 외장 GPU를 고르게 된다.
	ComPtr<IDXGIFactory6> factory6;
	if (SUCCEEDED(m_factory.As(&factory6)))
	{
		for (UINT i = 0; ; ++i)
		{
			ComPtr<IDXGIAdapter1> adapter;
			if (factory6->EnumAdapterByGpuPreference(
					i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) == DXGI_ERROR_NOT_FOUND)
			{
				break;
			}

			DXGI_ADAPTER_DESC1 desc = {};
			adapter->GetDesc1(&desc);

			// WARP(소프트웨어 래스터라이저)는 건너뛴다.
			if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
			{
				continue;
			}

			// 실제로 만들지는 않고 지원 여부만 확인한다. (마지막 인자 nullptr)
			if (SUCCEEDED(::D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
			{
				m_adapterName = desc.Description;
				LOG_INFO(L"어댑터 선택: %s (전용 VRAM %llu MB)",
					desc.Description, static_cast<unsigned long long>(desc.DedicatedVideoMemory / (1024 * 1024)));
				return adapter;
			}
		}
	}

	// IDXGIFactory6가 없는 경우의 대비책: 순서대로 훑어 첫 번째 지원 어댑터를 쓴다.
	for (UINT i = 0; ; ++i)
	{
		ComPtr<IDXGIAdapter1> adapter;
		if (m_factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND)
		{
			break;
		}

		DXGI_ADAPTER_DESC1 desc = {};
		adapter->GetDesc1(&desc);

		if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
		{
			continue;
		}

		if (SUCCEEDED(::D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
		{
			m_adapterName = desc.Description;
			LOG_INFO(L"어댑터 선택(대비책): %s", desc.Description);
			return adapter;
		}
	}

	return nullptr;
}

void D3D12Device::ConfigureInfoQueue() const
{
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> infoQueue;
	if (FAILED(m_device.As(&infoQueue)))
	{
		return;
	}

	// 오류/경고가 나면 그 자리에서 디버거가 멈춘다. 원인 추적이 훨씬 쉬워진다.
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

	// 실무상 무시해도 되는, 소음에 가까운 메시지는 걸러낸다.
	D3D12_MESSAGE_ID denyIds[] =
	{
		// 지정한 클리어 색과 실제 클리어 색이 다를 때 나는 경고.
		// 최적화 힌트일 뿐이라 동작에는 문제가 없다.
		D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
		D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
	};

	D3D12_INFO_QUEUE_FILTER filter = {};
	filter.DenyList.NumIDs = _countof(denyIds);
	filter.DenyList.pIDList = denyIds;
	infoQueue->AddStorageFilterEntries(&filter);

	LOG_INFO(L"디버그 InfoQueue 설정 완료 (오류/경고 발생 시 중단)");
#endif
}

void D3D12Device::CacheDescriptorSizes()
{
	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_descriptorSizes[i] = m_device->GetDescriptorHandleIncrementSize(
			static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
	}
}

UINT D3D12Device::GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	const int index = static_cast<int>(type);
	if (index < 0 || index >= D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)
	{
		return 0;
	}
	return m_descriptorSizes[index];
}

void D3D12Device::QueryFeatureLevel()
{
	// 높은 것부터 나열해 두면 런타임이 실제 지원 상한을 골라준다.
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels = {};
	featureLevels.NumFeatureLevels = _countof(levels);
	featureLevels.pFeatureLevelsRequested = levels;

	if (SUCCEEDED(m_device->CheckFeatureSupport(
			D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(featureLevels))))
	{
		m_featureLevel = featureLevels.MaxSupportedFeatureLevel;
	}
}

void D3D12Device::CheckTearingSupport()
{
	ComPtr<IDXGIFactory5> factory5;
	if (FAILED(m_factory.As(&factory5)))
	{
		return;
	}

	BOOL allowTearing = FALSE;
	if (SUCCEEDED(factory5->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
	{
		m_tearingSupported = (allowTearing == TRUE);
	}
}

UINT D3D12Device::QueryMsaaQualityLevels(DXGI_FORMAT format, UINT sampleCount) const
{
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS levels = {};
	levels.Format = format;
	levels.SampleCount = sampleCount;
	levels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;

	if (FAILED(m_device->CheckFeatureSupport(
			D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &levels, sizeof(levels))))
	{
		return 0;
	}

	// NumQualityLevels가 0이면 이 포맷/샘플 수 조합을 하드웨어가 지원하지 않는다는 뜻이다.
	return levels.NumQualityLevels;
}

void D3D12Device::ReportLiveObjects()
{
#if defined(_DEBUG)
	// 프로그램 종료 시점에 살아남은 D3D 객체를 출력 창에 찍는다.
	// 해제를 빠뜨린 리소스를 찾는 가장 확실한 방법이다.
	ComPtr<IDXGIDebug1> dxgiDebug;
	if (SUCCEEDED(::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
	{
		dxgiDebug->ReportLiveObjects(
			DXGI_DEBUG_ALL,
			static_cast<DXGI_DEBUG_RLO_FLAGS>(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
	}
#endif
}
