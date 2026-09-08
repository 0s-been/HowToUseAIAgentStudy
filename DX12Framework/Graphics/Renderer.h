// Renderer.h
// 디바이스 / 큐 / 스왑체인 / 파이프라인 / 프레임 자원을 한데 모아 프레임 흐름을 관리한다.
//
// 프레임 자원(FrameContext)을 백버퍼 장수만큼 두는 이유:
//   커맨드 얼로케이터와 상수 버퍼는 GPU가 다 쓰기 전에는 건드릴 수 없다.
//   하나뿐이면 CPU가 매 프레임 GPU를 기다려야 해서 병렬성이 사라진다.
//   장수만큼 두고 돌려쓰면 CPU는 N-1 프레임 앞서 나갈 수 있다.
//
// 한 프레임의 사용 흐름:
//   renderer.BeginFrame(clearColor);
//   renderer.SetPassConstants(camera, ...);
//   renderer.DrawMesh(mesh, worldMatrix, color);   // 여러 번 가능
//   renderer.EndFrame(vsync);

#pragma once
#include "../Core/stdafx.h"
#include "Camera.h"
#include "CommandQueue.h"
#include "ConstantBuffers.h"
#include "D3D12Device.h"
#include "GeometryGenerator.h"
#include "Mesh.h"
#include "PipelineState.h"
#include "RootSignature.h"
#include "SwapChain.h"
#include "UploadBuffer.h"

// 한 프레임이 배타적으로 쓰는 자원 묶음.
struct FrameContext
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;

	// 이 프레임이 마지막으로 제출한 작업의 펜스 값.
	// 같은 슬롯을 다시 쓰기 전에 이 값까지 GPU가 끝냈는지 확인한다.
	UINT64 fenceValue = 0;

	// 프레임마다 독립된 상수 버퍼를 둔다.
	// 공유하면 GPU가 아직 읽는 중인 값을 CPU가 덮어써 버린다.
	std::unique_ptr<UploadBuffer<ObjectConstants>> objectCB;
	std::unique_ptr<UploadBuffer<PassConstants>> passCB;
};

// 한 프레임에 그릴 수 있는 오브젝트 수 상한. 상수 버퍼 크기를 정한다.
constexpr UINT kMaxObjectsPerFrame = 256;

class Renderer
{
public:
	Renderer() = default;
	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	bool Initialize(HWND hWnd, UINT width, UINT height, bool enableDebugLayer);
	void Shutdown();

	bool Resize(UINT width, UINT height);

	// MeshData를 GPU 버퍼로 올린다.
	// 내부에서 복사 명령 제출과 완료 대기까지 처리하므로 초기화 시점에만 호출한다.
	bool CreateMesh(Mesh& outMesh, const MeshData& data, const wchar_t* debugName);

	// BeginFrame ~ EndFrame 사이에서만 그리기 명령을 기록할 수 있다.
	void BeginFrame(const DirectX::XMFLOAT4& clearColor);
	void SetPassConstants(const Camera& camera, float totalTime);
	void DrawMesh(const Mesh& mesh, const DirectX::XMMATRIX& world, const DirectX::XMFLOAT4& baseColor);
	void EndFrame(bool vsync);

	// 제출된 모든 GPU 작업이 끝날 때까지 대기한다.
	void WaitForGpu();

	// 방향광 설정. direction은 빛이 나아가는 방향이다.
	void SetDirectionalLight(const DirectX::XMFLOAT3& direction,
		const DirectX::XMFLOAT4& color,
		const DirectX::XMFLOAT4& ambient);

	// 와이어프레임 표시를 켜고 끈다. 다음 BeginFrame부터 반영된다.
	// (Renderer가 SetDirectionalLight처럼 몇 프레임에 한 번 바뀌는 렌더 상태를
	//  몇 개 들고 있는 기존 방식과 같은 자리에 둔다.)
	void SetWireframe(bool enabled) { m_wireframeEnabled = enabled; }
	bool IsWireframeEnabled() const { return m_wireframeEnabled; }

	D3D12Device& GetDevice() { return m_device; }
	CommandQueue& GetGraphicsQueue() { return m_graphicsQueue; }
	SwapChain& GetSwapChain() { return m_swapChain; }
	ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }

	float GetAspectRatio() const { return m_swapChain.GetAspectRatio(); }

private:
	bool CreateFrameResources();
	// 셰이더를 컴파일하고 루트 시그니처와 PSO를 만든다.
	bool CreateGraphicsPipeline(const std::wstring& shaderDirectory);

	D3D12Device m_device;
	CommandQueue m_graphicsQueue;
	SwapChain m_swapChain;

	RootSignature m_rootSignature;
	// 채우기 모드만 다른 PSO 두 개를 미리 만들어 두고 프레임마다 골라 쓴다.
	// (PSO는 불변 객체라 "지금만 와이어프레임으로" 같은 부분 변경이 불가능하다)
	PipelineState m_solidPipelineState;
	PipelineState m_wireframePipelineState;

	FrameContext m_frames[kFrameBufferCount];
	ComPtr<ID3D12GraphicsCommandList> m_commandList;

	// 이번 프레임이 쓰는 FrameContext 슬롯. 백버퍼 인덱스와 같이 간다.
	UINT m_currentFrameIndex = 0;
	// 이번 프레임에서 지금까지 그린 오브젝트 수. 상수 버퍼의 몇 번째 칸을 쓸지 정한다.
	UINT m_objectCount = 0;
	bool m_frameStarted = false;
	bool m_wireframeEnabled = false;

	DirectX::XMFLOAT3 m_lightDirection = { 0.577f, -0.577f, 0.577f };
	DirectX::XMFLOAT4 m_lightColor = { 1.0f, 0.97f, 0.92f, 1.0f };
	DirectX::XMFLOAT4 m_ambientColor = { 0.25f, 0.27f, 0.32f, 1.0f };
};
