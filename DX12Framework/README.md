# DX12Framework

DirectX 12 기본 프레임워크. 같은 솔루션의 `DirectXProj`(DirectX 11)와는 독립적인 별도 프로젝트다.

회전하는 큐브 하나를 그리는 것까지가 현재 범위이며, 그 과정에 DX12의 기본기
(디바이스 · 커맨드 큐 · 펜스 · 스왑체인 · 디스크립터 힙 · 루트 시그니처 · PSO · 리소스 상태 전이)가
빠짐없이 들어가 있다.

## 빌드와 실행

1. Visual Studio 2022로 저장소 루트의 `DirectXProj.sln`을 연다.
2. 솔루션 탐색기에서 **DX12Framework**를 우클릭 → **시작 프로젝트로 설정**.
3. 구성을 **Debug | x64**로 두고 빌드 후 실행한다.

요구 사항은 Visual Studio 2022(v143 툴셋)와 Windows 10 SDK, DirectX 12를 지원하는 GPU다.
외부 라이브러리 의존성은 없다. (`d3dx12.h`도 쓰지 않는다.)

셰이더는 실행 시점에 `D3DCompileFromFile`로 컴파일한다. 그래서 프로젝트 속성의
**디버깅 → 작업 디렉터리**가 `$(ProjectDir)`로 설정되어 있어야 `Shaders\` 상대 경로가 통한다.
(이 설정은 `.vcxproj`에 이미 들어 있다.)

### 조작

| 키 | 동작 |
|---|---|
| `Space` | 큐브 회전 정지 / 재개 |
| `V` | 수직 동기화 토글 |
| `ESC` | 종료 |

### 로그

실행하면 초기화 각 단계의 로그가 세 곳에 남는다.

- Visual Studio 출력 창
- 별도 콘솔 창
- 실행 폴더의 `DX12Framework.log` (UTF-8)

어댑터 이름, 기능 레벨, 디스크립터 칸 크기, 스왑체인 크기, 셰이더 컴파일 결과 등이 기록되므로
문제가 생겼을 때 어느 단계에서 멈췄는지 바로 알 수 있다.

## 파일 구조

```
DX12Framework/
├── Main.cpp                  wWinMain 진입점
├── Core/                     그래픽 API와 무관한 기반 코드
│   ├── stdafx.h              공통 include, lib 링크, kFrameBufferCount
│   ├── DXException.h/.cpp    HRESULT 실패 -> 예외 (ThrowIfFailed)
│   ├── Logger.h/.cpp         출력창 + 콘솔 + 파일 3중 로거
│   ├── Timer.h/.cpp          QPC 기반 델타/누적 시간, FPS
│   ├── Window.h/.cpp         Win32 창과 메시지 펌프 (D3D를 전혀 모른다)
│   └── Application.h/.cpp    수명주기와 메인 루프
├── Graphics/                 렌더링
│   ├── D3D12Device.h/.cpp    디버그 레이어, 어댑터 선택, 디바이스 생성
│   ├── CommandQueue.h/.cpp   커맨드 큐 + 펜스 동기화
│   ├── DescriptorHeap.h/.cpp 디스크립터 힙 래퍼 + 선형 할당자
│   ├── SwapChain.h/.cpp      스왑체인, RTV, 깊이 버퍼, 뷰포트, 리사이즈
│   ├── D3D12Helpers.h        구조체 생성 헬퍼 (d3dx12.h 대체)
│   ├── Shader.h/.cpp         HLSL 런타임 컴파일
│   ├── RootSignature.h/.cpp  루트 시그니처
│   ├── PipelineState.h/.cpp  PSO
│   ├── UploadBuffer.h        업로드 힙 템플릿 버퍼
│   ├── Vertex.h              정점 포맷과 입력 레이아웃
│   ├── ConstantBuffers.h     상수 버퍼 구조체 (HLSL과 짝을 이룸)
│   ├── Mesh.h/.cpp           디폴트 힙 정점/인덱스 버퍼
│   ├── GeometryFactory.h/.cpp 큐브 지오메트리 생성
│   ├── Camera.h/.cpp         뷰/투영 행렬
│   └── Renderer.h/.cpp       프레임 흐름과 그리기
└── Shaders/
    ├── Common.hlsli          VS/PS 공용 cbuffer와 구조체
    ├── Basic_VS.hlsl
    └── Basic_PS.hlsl
```

의존 방향은 `Core → (없음)`, `Graphics → Core`, `Application → Core + Graphics` 한 방향이다.
`Window`가 D3D를 전혀 모르는 것도 같은 이유다. 창 코드와 그래픽 코드를 따로 고칠 수 있다.

## 한 프레임의 흐름

```cpp
renderer.BeginFrame(clearColor);              // 펜스 대기 → 얼로케이터 Reset → 배리어 → 클리어
renderer.SetPassConstants(camera, totalTime); // b1 바인딩 (프레임당 1회)
renderer.DrawMesh(mesh, world, color);        // b0 바인딩 + DrawIndexedInstanced (여러 번 가능)
renderer.EndFrame(vsync);                     // 배리어 → Close → Execute → Signal → Present
```

`BeginFrame`이 `WaitForFenceValue`로 대기하는 지점이 **CPU가 GPU보다 너무 앞서가지 못하게 막는 곳**이다.
프레임 자원을 백버퍼 장수(3개)만큼 두었기 때문에 CPU는 최대 2프레임 앞서 나갈 수 있다.

## 알아 둘 만한 설계 판단

- **상수 버퍼는 루트 디스크립터(Root CBV) 2개** (`b0` 오브젝트, `b1` 패스).
  텍스처가 없는 동안은 셰이더 가시 디스크립터 힙을 만들 필요조차 없어 코드가 크게 단순해진다.
- **상수 버퍼는 프레임마다 별도**. 공유하면 GPU가 읽는 중인 값을 CPU가 덮어쓴다.
- **행렬 규약**: CPU에서 `XMMatrixTranspose`로 전치해 올리고, HLSL에서는 행벡터 규약
  `mul(float4(pos,1), gWorld)`를 쓴다. `XMMATRIX`의 행 우선 저장과 HLSL의 열 우선 해석이
  서로 상쇄되어 셰이더에서는 원래 행렬이 그대로 보인다.
- **정점/인덱스는 디폴트 힙**, 상수 버퍼는 업로드 힙. 전자는 임시 업로드 버퍼를 거쳐 복사하며,
  그 임시 버퍼는 GPU 복사가 끝난 뒤에야(`Flush` 이후) 해제할 수 있다.
- **외부 의존성 없음**. `d3dx12.h` 대신 `D3D12Helpers.h`에 필요한 것만 직접 두었다.

## 여기서 확장하려면

| 하고 싶은 것 | 손댈 곳 |
|---|---|
| 도형 추가 (구, 평면, 원기둥) | `GeometryFactory`에 함수 추가 |
| 텍스처 | `DescriptorHeap`을 `CBV_SRV_UAV` + `shaderVisible=true`로 생성하고, 루트 시그니처에 디스크립터 테이블과 정적 샘플러 추가 |
| 카메라 조작 | `Camera`에 이동/회전 메서드 추가, `Application::Update`에서 입력 처리 |
| 오브젝트 여러 개 | `Application`이 오브젝트 목록을 들고 `DrawMesh`를 반복 호출 (프레임당 상한은 `kMaxObjectsPerFrame`) |
| 반투명 렌더링 | 블렌딩을 켠 PSO를 하나 더 만들고 그리기 순서를 분리 |
| MSAA | 플립 모델 백버퍼에는 직접 걸 수 없다. 별도 렌더 타겟에 그린 뒤 `ResolveSubresource`로 옮겨야 한다 |

단계별로 무엇을 왜 그렇게 했는지는 [`../Docs/DX12_Framework_WorkLog.md`](../Docs/DX12_Framework_WorkLog.md)에 정리해 두었다.
