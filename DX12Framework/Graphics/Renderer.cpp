#include "Renderer.h"

#include "../Core/DXException.h"
#include "../Core/Logger.h"
#include "D3D12Helpers.h"
#include "Shader.h"
#include "Vertex.h"

using namespace DirectX;

namespace
{
	// HLSL이 행렬 X를 보게 하려면 transpose(X)를 올려야 한다.
	// (XMMATRIX는 행 우선 저장, HLSL 상수 버퍼는 열 우선 해석이라 서로 상쇄된다.)
	XMFLOAT4X4 StoreForShader(const XMMATRIX& matrix)
	{
		XMFLOAT4X4 result;
		XMStoreFloat4x4(&result, XMMatrixTranspose(matrix));
		return result;
	}
}

Renderer::~Renderer()
{
	Shutdown();
}

bool Renderer::Initialize(HWND hWnd, UINT width, UINT height, bool enableDebugLayer)
{
	if (!m_device.Initialize(enableDebugLayer))
	{
		return false;
	}

	if (!m_graphicsQueue.Initialize(m_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, L"GraphicsQueue"))
	{
		return false;
	}

	if (!m_swapChain.Initialize(&m_device, &m_graphicsQueue, hWnd, width, height))
	{
		return false;
	}

	if (!CreateFrameResources())
	{
		return false;
	}

	if (!CreateGraphicsPipeline(L"Shaders\\"))
	{
		return false;
	}

	m_currentFrameIndex = m_swapChain.GetCurrentBackBufferIndex();

	LOG_INFO(L"렌더러 초기화 완료");
	return true;
}

bool Renderer::CreateFrameResources()
{
	try
	{
		for (UINT i = 0; i < kFrameBufferCount; ++i)
		{
			ThrowIfFailed(m_device.Get()->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_frames[i].commandAllocator)));

			wchar_t name[40] = {};
			swprintf_s(name, L"CommandAllocator[%u]", i);
			m_frames[i].commandAllocator->SetName(name);

			m_frames[i].fenceValue = 0;

			m_frames[i].objectCB = std::make_unique<UploadBuffer<ObjectConstants>>();
			swprintf_s(name, L"ObjectCB[%u]", i);
			if (!m_frames[i].objectCB->Initialize(m_device.Get(), kMaxObjectsPerFrame, true, name))
			{
				return false;
			}

			m_frames[i].passCB = std::make_unique<UploadBuffer<PassConstants>>();
			swprintf_s(name, L"PassCB[%u]", i);
			if (!m_frames[i].passCB->Initialize(m_device.Get(), 1, true, name))
			{
				return false;
			}
		}

		// 커맨드 리스트는 1개만 두고 매 프레임 얼로케이터만 바꿔가며 Reset한다.
		// (기록은 CPU 한 스레드에서만 하므로 여러 개가 필요 없다.)
		ThrowIfFailed(m_device.Get()->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_frames[0].commandAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(&m_commandList)));
		m_commandList->SetName(L"MainCommandList");

		// CreateCommandList는 '기록 중' 상태로 만들어 준다.
		// BeginFrame이 항상 Reset으로 시작할 수 있도록 여기서 한 번 닫아 둔다.
		ThrowIfFailed(m_commandList->Close());

		LOG_INFO(L"프레임 자원 생성 완료 (프레임 %u개, 프레임당 오브젝트 최대 %u개)",
			kFrameBufferCount, kMaxObjectsPerFrame);
		return true;
	}
	catch (const DxException& e)
	{
		LOG_ERROR(L"프레임 자원 생성 실패: %s", e.ToString().c_str());
		return false;
	}
}

bool Renderer::CreateGraphicsPipeline(const std::wstring& shaderDirectory)
{
	Shader vertexShader;
	Shader pixelShader;

	if (!vertexShader.CompileFromFile(shaderDirectory + L"Basic_VS.hlsl", "main", "vs_5_1"))
	{
		return false;
	}
	if (!pixelShader.CompileFromFile(shaderDirectory + L"Basic_PS.hlsl", "main", "ps_5_1"))
	{
		return false;
	}

	if (!m_rootSignature.InitializeDefault(m_device.Get()))
	{
		return false;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = PipelineState::MakeDefaultDesc();
	desc.pRootSignature = m_rootSignature.Get();
	desc.InputLayout.pInputElementDescs = kVertexInputLayout;
	desc.InputLayout.NumElements = kVertexInputLayoutCount;
	desc.VS = vertexShader.GetBytecode();
	desc.PS = pixelShader.GetBytecode();
	// PSO의 포맷은 실제 렌더 타겟/깊이 버퍼와 정확히 일치해야 한다.
	desc.RTVFormats[0] = SwapChain::kBackBufferFormat;
	desc.DSVFormat = SwapChain::kDepthStencilFormat;

	// CreateGraphicsPipelineState는 셰이더 바이트코드를 내부로 복사한다.
	// 따라서 지역 변수인 vertexShader/pixelShader가 여기서 소멸해도 문제없다.
	return m_pipelineState.Initialize(m_device.Get(), desc, L"BasicPSO");
}

bool Renderer::CreateMesh(Mesh& outMesh, const MeshData& data, const wchar_t* debugName)
{
	try
	{
		// 초기화 시점이므로 어느 얼로케이터를 써도 무방하다.
		FrameContext& frame = m_frames[0];
		ThrowIfFailed(frame.commandAllocator->Reset());
		ThrowIfFailed(m_commandList->Reset(frame.commandAllocator.Get(), nullptr));

		// 이 호출은 복사 명령을 '기록'만 한다.
		if (!outMesh.Initialize(m_device.Get(), m_commandList.Get(), data.vertices, data.indices, debugName))
		{
			m_commandList->Close();
			return false;
		}

		ThrowIfFailed(m_commandList->Close());
		m_graphicsQueue.ExecuteCommandList(m_commandList.Get());

		// 복사가 실제로 끝날 때까지 기다린 뒤라야 임시 업로드 버퍼를 버릴 수 있다.
		m_graphicsQueue.Flush();
		outMesh.DisposeUploaders();

		return true;
	}
	catch (const DxException& e)
	{
		LOG_ERROR(L"메시 업로드 실패: %s", e.ToString().c_str());
		return false;
	}
}

void Renderer::Shutdown()
{
	// 어떤 것도 해제하기 전에 GPU를 반드시 비운다.
	if (m_graphicsQueue.Get() != nullptr)
	{
		m_graphicsQueue.Flush();
	}

	m_pipelineState.Shutdown();
	m_rootSignature.Shutdown();

	m_commandList.Reset();
	for (UINT i = 0; i < kFrameBufferCount; ++i)
	{
		m_frames[i].commandAllocator.Reset();
		m_frames[i].objectCB.reset();
		m_frames[i].passCB.reset();
		m_frames[i].fenceValue = 0;
	}

	m_swapChain.Shutdown();
	m_graphicsQueue.Shutdown();
	m_device.Shutdown();
}

void Renderer::WaitForGpu()
{
	if (m_graphicsQueue.Get() == nullptr)
	{
		return;
	}

	m_graphicsQueue.Flush();

	// 큐를 비웠으므로 모든 프레임 슬롯을 다시 써도 안전하다.
	for (UINT i = 0; i < kFrameBufferCount; ++i)
	{
		m_frames[i].fenceValue = 0;
	}
}

bool Renderer::Resize(UINT width, UINT height)
{
	// SwapChain::Resize도 내부에서 Flush하지만, 프레임 슬롯의 펜스 값까지 정리해야 한다.
	WaitForGpu();
	return m_swapChain.Resize(width, height);
}

void Renderer::SetDirectionalLight(const XMFLOAT3& direction, const XMFLOAT4& color, const XMFLOAT4& ambient)
{
	// 셰이더에서 다시 정규화하지만, 값 자체를 정규화해 두는 편이 안전하다.
	XMStoreFloat3(&m_lightDirection, XMVector3Normalize(XMLoadFloat3(&direction)));
	m_lightColor = color;
	m_ambientColor = ambient;
}

void Renderer::BeginFrame(const XMFLOAT4& clearColor)
{
	m_currentFrameIndex = m_swapChain.GetCurrentBackBufferIndex();
	FrameContext& frame = m_frames[m_currentFrameIndex];

	// 이 슬롯을 마지막으로 쓴 프레임이 GPU에서 끝났는지 확인한다.
	// 끝나지 않았다면 여기서 대기한다. (CPU가 GPU보다 너무 앞서가는 것을 막는 지점)
	m_graphicsQueue.WaitForFenceValue(frame.fenceValue);

	// 얼로케이터를 Reset하면 이전에 기록된 명령이 통째로 버려진다.
	frame.commandAllocator->Reset();
	m_commandList->Reset(frame.commandAllocator.Get(), m_pipelineState.Get());

	m_objectCount = 0;

	// 백버퍼를 '표시용'에서 '렌더 타겟'으로 전이시킨다.
	// 이 배리어를 빠뜨리면 디버그 레이어가 즉시 오류를 낸다.
	const D3D12_RESOURCE_BARRIER toRenderTarget = DX::TransitionBarrier(
		m_swapChain.GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &toRenderTarget);

	// 뷰포트와 가위 영역은 커맨드 리스트를 Reset할 때마다 초기화되므로 매 프레임 다시 설정한다.
	const D3D12_VIEWPORT viewport = m_swapChain.GetViewport();
	const D3D12_RECT scissor = m_swapChain.GetScissorRect();
	m_commandList->RSSetViewports(1, &viewport);
	m_commandList->RSSetScissorRects(1, &scissor);

	const D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain.GetCurrentBackBufferView();
	const D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_swapChain.GetDepthStencilView();
	m_commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	const float color[4] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
	m_commandList->ClearRenderTargetView(rtv, color, 0, nullptr);
	m_commandList->ClearDepthStencilView(
		dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// 루트 시그니처도 Reset으로 지워지므로 매 프레임 다시 설정해야 한다.
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	// D3D12_PRIMITIVE_TOPOLOGY는 D3D_PRIMITIVE_TOPOLOGY의 typedef이므로
	// 열거값 접두사는 D3D12_가 아니라 D3D_다.
	// (PSO에 넣는 D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE은 이름이 비슷하지만 다른 열거형이다.)
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_frameStarted = true;
}

void Renderer::SetPassConstants(const Camera& camera, float totalTime)
{
	if (!m_frameStarted)
	{
		return;
	}

	FrameContext& frame = m_frames[m_currentFrameIndex];

	PassConstants constants = {};
	constants.viewProj = StoreForShader(camera.GetViewProj());
	constants.eyePosW = camera.GetPosition();
	constants.totalTime = totalTime;
	constants.lightDirection = m_lightDirection;
	constants.padding = 0.0f;
	constants.lightColor = m_lightColor;
	constants.ambientColor = m_ambientColor;

	frame.passCB->CopyData(0, constants);

	// 루트 파라미터 1번 = b1 (패스 상수). 프레임에 한 번만 바인딩하면 된다.
	m_commandList->SetGraphicsRootConstantBufferView(1, frame.passCB->GetGpuAddress(0));
}

void Renderer::DrawMesh(const Mesh& mesh, const XMMATRIX& world, const XMFLOAT4& baseColor)
{
	if (!m_frameStarted || !mesh.IsValid())
	{
		return;
	}

	if (m_objectCount >= kMaxObjectsPerFrame)
	{
		LOG_WARN(L"한 프레임 오브젝트 상한(%u)을 넘었다. 이번 오브젝트는 건너뛴다.", kMaxObjectsPerFrame);
		return;
	}

	FrameContext& frame = m_frames[m_currentFrameIndex];

	ObjectConstants constants = {};
	constants.world = StoreForShader(world);

	// 법선 변환 행렬은 월드 행렬의 역전치다. 비균등 스케일에서도 법선이 표면에 수직으로 남는다.
	const XMMATRIX worldInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, world));
	constants.worldInvTranspose = StoreForShader(worldInvTranspose);

	constants.baseColor = baseColor;

	const UINT slot = m_objectCount;
	frame.objectCB->CopyData(slot, constants);

	// 루트 파라미터 0번 = b0 (오브젝트 상수). 오브젝트마다 다른 주소를 가리킨다.
	m_commandList->SetGraphicsRootConstantBufferView(0, frame.objectCB->GetGpuAddress(slot));

	mesh.Draw(m_commandList.Get());

	++m_objectCount;
}

void Renderer::EndFrame(bool vsync)
{
	if (!m_frameStarted)
	{
		return;
	}

	FrameContext& frame = m_frames[m_currentFrameIndex];

	// 화면에 내보내려면 다시 PRESENT 상태여야 한다.
	const D3D12_RESOURCE_BARRIER toPresent = DX::TransitionBarrier(
		m_swapChain.GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &toPresent);

	m_commandList->Close();

	// 제출과 동시에 이 프레임의 완료 표식을 받아 둔다.
	// 다음에 같은 슬롯을 쓸 때 이 값을 기다리게 된다.
	frame.fenceValue = m_graphicsQueue.ExecuteCommandList(m_commandList.Get());

	m_swapChain.Present(vsync);
	m_frameStarted = false;
}
