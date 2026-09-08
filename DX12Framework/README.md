# DX12Framework

DirectX 12 기본 프레임워크. 체커 무늬 바닥 위에서 회전하는 큐브를 WASD와 마우스로 돌아다니며
볼 수 있는 것까지가 현재 범위이며, 그 과정에 DX12의 기본기가 빠짐없이 들어가 있다 —
디바이스 · 커맨드 큐 · 펜스 · 스왑체인(+MSAA) · 디스크립터 힙 · 루트 시그니처 · PSO ·
리소스 상태 전이 · 업로드/디폴트 힙 구분 · 프레임 자원 다중화.

## 빌드와 실행

1. Visual Studio 2022로 저장소 루트의 `DX12Framework.sln`을 연다.
2. 구성을 **Debug | x64**로 두고 빌드 후 실행한다.

요구 사항은 Visual Studio 2022(v143 툴셋)와 Windows 10 SDK, DirectX 12를 지원하는 GPU다.
외부 라이브러리 의존성은 없다. (`d3dx12.h`도 쓰지 않는다.)

셰이더는 실행 시점에 `D3DCompileFromFile`로 컴파일한다. 그래서 프로젝트 속성의
**디버깅 → 작업 디렉터리**가 `$(ProjectDir)`로 설정되어 있어야 `Shaders\` 상대 경로가 통한다.
(이 설정은 `.vcxproj`에 이미 들어 있다.)

### 조작

| 입력 | 동작 |
|---|---|
| `W` `A` `S` `D` | 앞 / 왼쪽 / 뒤 / 오른쪽 이동 |
| `Q` `E` | 아래 / 위로 이동 |
| `Shift` (누른 채) | 이동 속도 3배 |
| **마우스 우클릭 + 드래그** | 카메라 시야 회전 (드래그 중 커서 숨김) |
| `Space` | 큐브 회전 정지 / 재개 |
| `Tab` | 솔리드 / 와이어프레임 토글 |
| `V` | 수직 동기화 토글 |
| `ESC` | 종료 |

### 로그

실행하면 초기화 각 단계의 로그가 세 곳에 남는다.

- Visual Studio 출력 창
- 별도 콘솔 창
- 실행 폴더의 `DX12Framework.log` (UTF-8)

어댑터 이름, 기능 레벨, 디스크립터 칸 크기, 스왑체인 크기, MSAA 샘플 수, 셰이더 컴파일
결과 등이 기록되므로 문제가 생겼을 때 어느 단계에서 멈췄는지 바로 알 수 있다.

## 파일 구조

```
DX12Framework/
├── Main.cpp                  wWinMain 진입점
├── Core/                     그래픽 API와 무관한 기반 코드
│   ├── stdafx.h              공통 include, lib 링크, kFrameBufferCount
│   ├── DXException.h/.cpp    HRESULT 실패 -> 예외 (ThrowIfFailed)
│   ├── Logger.h/.cpp         출력창 + 콘솔 + 파일 3중 로거
│   ├── Timer.h/.cpp          QPC 기반 델타/누적 시간, FPS
│   ├── Window.h/.cpp         Win32 창과 메시지 펌프 (D3D도 입력도 모른다)
│   └── Application.h/.cpp    수명주기와 메인 루프
├── Input/
│   └── InputReader.h/.cpp    키보드/마우스 상태 수집, 프레임 단위 질의
├── Graphics/                 렌더링
│   ├── D3D12Device.h/.cpp    디버그 레이어, 어댑터 선택, 디바이스 생성, MSAA 지원 조회
│   ├── CommandQueue.h/.cpp   커맨드 큐 + 펜스 동기화
│   ├── DescriptorHeap.h/.cpp 디스크립터 힙 래퍼 + 선형 할당자
│   ├── SwapChain.h/.cpp      스왑체인, RTV, MSAA 색상 타겟, 깊이 버퍼, 뷰포트, 리사이즈
│   ├── D3D12Helpers.h        구조체 생성 헬퍼 (d3dx12.h 대체)
│   ├── Shader.h/.cpp         HLSL 런타임 컴파일
│   ├── RootSignature.h/.cpp  루트 시그니처
│   ├── PipelineState.h/.cpp  PSO
│   ├── UploadBuffer.h        업로드 힙 템플릿 버퍼
│   ├── Vertex.h              정점 포맷과 입력 레이아웃
│   ├── ConstantBuffers.h     상수 버퍼 구조체 (HLSL과 짝을 이룸)
│   ├── Mesh.h/.cpp           디폴트 힙 정점/인덱스 버퍼
│   ├── GeometryGenerator.h/.cpp 도형의 CPU 정점/인덱스 데이터 생성 (색은 다루지 않는다)
│   ├── MeshFactory.h/.cpp    메시 생성·캐시·수명 관리 (메시의 소유자)
│   ├── Camera.h/.cpp         1인칭 카메라 (yaw/pitch, 이동/회전)
│   └── Renderer.h/.cpp       프레임 흐름과 그리기, 솔리드/와이어프레임 PSO 전환
└── Shaders/
    ├── Common.hlsli          VS/PS 공용 cbuffer와 구조체
    ├── Basic_VS.hlsl
    └── Basic_PS.hlsl         조명 + 절차적(필터링된) 체커 무늬
```

의존 방향은 `Core → (없음)`, `Graphics → Core`, `Input → Core`,
`Application → Core + Graphics + Input` 한 방향이다.
`Window`가 D3D도 입력도 모르는 것이 같은 이유다. `Window`는 받은 메시지를 콜백으로 흘려보내기만
하고, 그것을 입력으로 해석하는 일은 `InputReader`가 한다. 창 코드 · 그래픽 코드 · 입력 코드를
따로 고칠 수 있다.

### 입력 처리 순서

`InputReader`는 프레임당 호출 순서가 정해져 있다.

```cpp
m_input->BeginFrame();          // 이전 프레임 상태 보관, 마우스 이동량 0으로 초기화
m_window->ProcessMessages();    // 이 사이에 ProcessMessage가 여러 번 불린다
// 이후 IsKeyDown / WasKeyPressed / GetMouseDeltaX ... 질의
```

`BeginFrame`을 메시지 펌프보다 **먼저** 부르는 것이 핵심이다.
그래야 `WasKeyPressed`(눌린 그 프레임만 true)가 정확히 한 프레임만 참이 된다.

이동은 `deltaTime`에 비례시키고(프레임률과 무관한 속도), 회전은 마우스가 움직인 픽셀 수에만
비례시킨다(이동량 자체가 이미 이번 프레임 값이라 `deltaTime`을 곱하면 감도가 프레임률에 따라
달라진다).

## 한 프레임의 흐름

```cpp
renderer.BeginFrame(clearColor);              // 펜스 대기 → 얼로케이터 Reset → (MSAA 타겟/백버퍼) 클리어
renderer.SetPassConstants(camera, totalTime); // b1 바인딩 (프레임당 1회)
renderer.DrawMesh(mesh, world, color);        // b0 바인딩 + DrawIndexedInstanced (여러 번 가능)
renderer.EndFrame(vsync);                     // (MSAA면 Resolve) → Close → Execute → Signal → Present
```

`BeginFrame`이 `WaitForFenceValue`로 대기하는 지점이 **CPU가 GPU보다 너무 앞서가지 못하게
막는 곳**이다. 프레임 자원을 백버퍼 장수(3개)만큼 두었기 때문에 CPU는 최대 2프레임 앞서
나갈 수 있다.

### MSAA가 켜져 있을 때

플립 모델(`DXGI_SWAP_EFFECT_FLIP_DISCARD`) 백버퍼는 멀티샘플을 직접 지원하지 않는다.
하드웨어가 4x MSAA를 지원하면(`D3D12Device::QueryMsaaQualityLevels`로 확인),
`SwapChain`이 별도의 멀티샘플 렌더 타겟을 만들어 거기에 그린 뒤, `EndFrame`에서
`ResolveSubresource`로 그 내용을 백버퍼에 내려받는다(다운샘플). 지원하지 않는 하드웨어에서는
예전처럼 백버퍼에 직접 그린다 — 이 차이는 `SwapChain::GetColorTargetView()`가 감춰서
`Renderer`가 최소한만 분기한다.

## 알아 둘 만한 설계 판단

- **상수 버퍼는 루트 디스크립터(Root CBV) 2개** (`b0` 오브젝트, `b1` 패스).
  텍스처가 없는 동안은 셰이더 가시 디스크립터 힙을 만들 필요조차 없어 코드가 크게 단순해진다.
- **상수 버퍼는 프레임마다 별도**. 공유하면 GPU가 읽는 중인 값을 CPU가 덮어쓴다.
- **행렬 규약**: CPU에서 `XMMatrixTranspose`로 전치해 올리고, HLSL에서는 행벡터 규약
  `mul(float4(pos,1), gWorld)`를 쓴다.
- **정점/인덱스는 디폴트 힙**, 상수 버퍼는 업로드 힙. 전자는 임시 업로드 버퍼를 거쳐 복사하며,
  그 임시 버퍼는 GPU 복사가 끝난 뒤에야(`Flush` 이후) 해제할 수 있다.
- **외부 의존성 없음**. `d3dx12.h` 대신 `D3D12Helpers.h`에 필요한 것만 직접 두었다.
- **도형 데이터와 GPU 자원을 분리**. `GeometryGenerator`는 순수 CPU 계산만 하고,
  `MeshFactory`가 그것을 GPU에 올려 이름으로 관리한다. 아래 참고.
- **도형은 색을 갖지 않는다**. 정점 색이 아니라 `Renderer::DrawMesh`에 넘기는 값으로
  픽셀 셰이더가 색/무늬를 결정한다. 아래 "체커 무늬는 어떻게 그려지는가" 참고.
- **PSO는 솔리드/와이어프레임 두 개를 미리 만들어 둔다**. PSO는 불변 객체라 "지금 프레임만
  채우기 모드를 바꾼다"가 불가능하기 때문이다. `Renderer::SetWireframe(bool)`으로 어느 것을
  쓸지 고른다.

## 메시는 MeshFactory를 통해서만 만든다

메시를 쓰는 쪽에서 직접 만들면 세 가지가 호출부에 흩어진다.
도형 데이터 계산, GPU 업로드(제출 → 대기 → 임시 버퍼 해제), 그리고 GPU 작업이 끝난 뒤로 맞춰야 하는 소멸 시점.
팩토리가 이 셋을 한곳에서 처리하므로 호출부는 이렇게만 쓰면 된다.

```cpp
m_meshFactory.Initialize(m_renderer.get());

Mesh* ground = m_meshFactory.CreateGrid(L"GroundGrid", 40.0f, 40.0f, 20, 20);
Mesh* cube   = m_meshFactory.CreateBox(L"Cube", 1.5f, 1.5f, 1.5f);
Mesh* found  = m_meshFactory.Find(L"Cube");   // 같은 포인터
```

- **소유자는 팩토리다.** 호출부가 받는 것은 관찰용 포인터이며 해제하지 않는다.
- **같은 이름은 재사용된다.** 여러 오브젝트가 같은 도형을 공유할 때 중복 업로드가 없다.
- 내부적으로 `unique_ptr`에 담는다. 맵이 재해싱되어도 이미 나눠 준 `Mesh*`가 무효가 되지 않아야 하기 때문이다.
- `Shutdown()`은 반드시 `Renderer::WaitForGpu()` **뒤에** 불러야 한다. GPU가 아직 그 메시를 그리고 있을 수 있다.
- `CreateGrid`/`CreatePlane`은 색 인자를 받지 않는다. 격자는 흰색 정점만 가진 순수 지오메트리다.

새 도형은 `GeometryGenerator`에 `MeshData`를 돌려주는 함수를 하나 더하고,
`MeshFactory::Create(name, data)`로 등록하면 끝난다. 렌더링 코드는 건드릴 필요가 없다.

## 체커 무늬는 어떻게 그려지는가

바닥 격자는 색을 갖지 않는 순수 지오메트리다. 실제 체커 무늬는 `Renderer::DrawMesh`에
넘기는 값으로 픽셀 셰이더(`Basic_PS.hlsl`의 `FilteredChecker`)가 매 픽셀 계산한다.

```cpp
// checkerCellSize가 0(기본값)이면 baseColor만 쓴다(큐브처럼 단색 오브젝트).
// 0보다 크면 baseColor/checkerColorB 두 색으로 체커를 그린다.
renderer.DrawMesh(*groundMesh, XMMatrixIdentity(), colorA, colorB, /*cellSize=*/2.0f);
```

정점에 색을 미리 구워 넣지 않고 매 픽셀 계산하는 이유는 안티앨리어싱 때문이다. 체커
칸이 화면에서 카메라로부터 먼 곳(수평선 근처)에서 1픽셀보다 작아지면, MSAA의 고정된
샘플 개수로는 그 무늬를 다 담아내지 못해 카메라가 조금만 움직여도 경계가 반짝인다
(minification aliasing — MSAA 샘플 수를 늘려도 정도만 줄 뿐 없어지지 않는다).
`FilteredChecker`는 픽셀 셰이더에서 `ddx`/`ddy`로 화면 공간 도함수(무늬가 1픽셀당 얼마나
변하는지)를 구해 그 폭만큼 체커를 박스 필터로 미리 흐린다. 무늬가 화면에서 작아질수록
필터 폭이 넓어져 자연스럽게 중간 회색(0.5)으로 수렴하므로, 그 근본 원인이 사라진다.

## 여기서 확장하려면

| 하고 싶은 것 | 손댈 곳 |
|---|---|
| 도형 추가 (구, 원기둥 등) | `GeometryGenerator`에 함수 하나 추가 → `MeshFactory::Create`로 등록 |
| 텍스처 | `DescriptorHeap`을 `CBV_SRV_UAV` + `shaderVisible=true`로 생성하고, 루트 시그니처에 디스크립터 테이블과 정적 샘플러 추가 |
| 이동 속도/감도 조정 | `Application`의 `m_cameraSpeed`, `m_mouseSensitivity` |
| 게임패드, 키 리매핑 | `InputReader`에 상태 추가 후 `Application::UpdateCamera`에서 질의 |
| 오브젝트 여러 개 | `Application`이 오브젝트 목록을 들고 `DrawMesh`를 반복 호출 (프레임당 상한은 `kMaxObjectsPerFrame`) |
| 반투명 렌더링 | 블렌딩을 켠 PSO를 하나 더 만들고 그리기 순서를 분리 |
| MSAA 샘플 수 조정 | `SwapChain::kDesiredMsaaSampleCount` (기본 4) |

단계별로 무엇을 왜 그렇게 했는지는 [`../Docs/DX12_Framework_WorkLog.md`](../Docs/DX12_Framework_WorkLog.md)에 정리해 두었다.
