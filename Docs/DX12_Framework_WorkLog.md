# DirectX 12 프레임워크 작업 로그

DirectX 12 기본 프레임워크를 단계적으로 구축한 기록이다. 각 단계마다 무엇을 만들었고
왜 그렇게 결정했는지를 남긴다. DX12를 처음 보는 상태에서 순서대로 읽으면 전체 그림이 잡히게
쓰는 것을 목표로 한다.

- 대상 솔루션: `DX12Framework.sln`
- 프로젝트: `DX12Framework/DX12Framework.vcxproj` (v143 / C++20 / x64·Win32)
- 셰이더 처리 방식: `D3DCompileFromFile`을 이용한 **런타임 컴파일**
- 작업 브랜치: `claude/directx12-framework-setup-g32hsz`

> 참고: 초기 작업 당시 저장소에는 DirectX 11 기반의 별도 프로젝트(`DirectXProj`)가 함께
> 있었다. DX12Framework는 처음부터 그것과 무관한 새 프로젝트로 만들었고, 이후 앞으로의
> 작업을 DX12로 일원화하면서 그 DX11 프로젝트는 저장소에서 제거했다(10단계). 그래서
> 1~9단계 설명 중 "기존 DirectXProj는 그대로 둔다"는 취지의 문장이 나오는 것은 당시
> 상황을 그대로 남긴 것이고, 현재 저장소에는 DX12Framework 하나만 있다.

---

## 1단계 — 프로젝트 뼈대와 Core 계층

### 목표
DirectX를 붙이기 전에, 창을 띄우고 루프를 돌리고 로그를 남기는 토대를 먼저 만든다.
이 단계만으로도 빌드와 실행이 되어야 한다.

### 추가한 파일
| 파일 | 역할 |
|---|---|
| `Core/stdafx.h` | Windows/STL 공통 include, `ComPtr` 별칭 |
| `Core/DXException.h/.cpp` | `HRESULT` 실패를 예외로 바꾸는 `ThrowIfFailed` |
| `Core/Logger.h/.cpp` | 출력 창 + 콘솔 + UTF-8 로그 파일 3중 출력 로거 |
| `Core/Timer.h/.cpp` | QPC 기반 델타/누적 시간, FPS 집계 |
| `Core/Window.h/.cpp` | Win32 창 생성, 메시지 펌프, 리사이즈 콜백 |
| `Main.cpp` | `wWinMain` 진입점 |
| `DX12Framework.vcxproj` / `.filters` | 프로젝트 정의 |
| `.editorconfig` | 코드 스타일 고정 (탭 4칸) |

### 결정 사항과 이유
- **PCH 미사용** — `stdafx.h`는 단순 공통 헤더로만 쓴다. 빌드 설정이 단순해지고
  파일을 추가할 때 실수할 여지가 줄어든다.
- **`/utf-8` 컴파일 옵션** — 소스에 한글 주석이 있어 소스/실행 문자 집합을 UTF-8로 고정했다.
- **`Window`는 D3D를 전혀 모른다** — 창 크기 변경과 원시 메시지를 콜백으로만 알린다.
  덕분에 창 코드와 그래픽/입력 코드를 서로 독립적으로 수정할 수 있다. (이 원칙은 8단계에서
  입력 처리를 분리할 때도 그대로 이어진다.)
- **리사이즈는 드래그가 끝날 때 한 번만** — 테두리를 드래그하는 동안에는 `WM_SIZE`가 수십 번
  발생한다. `WM_ENTERSIZEMOVE`/`WM_EXITSIZEMOVE`로 구간을 잡아 드래그가 끝난 시점에 한 번만
  콜백을 호출한다.
- **로그 매크로에 서식 문자열까지 `__VA_ARGS__`로 넘김** — `__VA_ARGS__`가 절대 비지 않게 되어
  MSVC의 기존 전처리기와 `/Zc:preprocessor` 양쪽에서 모두 동작한다.
- **로그 파일에 UTF-8 BOM 기록** — 메모장으로 열어도 한글이 깨지지 않는다.

---

## 2단계 — D3D12 디바이스 계층 (Device / CommandQueue / DescriptorHeap)

### 목표
D3D11에서는 `D3D11CreateDeviceAndSwapChain` 한 번이면 끝나던 초기화가, D3D12에서는
디바이스·커맨드 큐·스왑체인으로 완전히 쪼개져 있다. 그중 화면과 무관한 "디바이스" 부분을
먼저 세운다.

### 추가한 파일
| 파일 | 역할 |
|---|---|
| `Graphics/D3D12Device.h/.cpp` | 디버그 레이어, 어댑터 선택, `ID3D12Device` 생성, 기능 조회 |
| `Graphics/CommandQueue.h/.cpp` | 커맨드 큐 + 펜스 동기화 (`Signal`/`WaitForFenceValue`/`Flush`) |
| `Graphics/DescriptorHeap.h/.cpp` | 디스크립터 힙 래퍼 + 선형 할당자 |

`Core/stdafx.h`에 `d3d12.h` / `dxgi1_6.h` / `DirectXMath.h` include와
`d3d12.lib` / `dxgi.lib` / `dxguid.lib` 링크, `kFrameBufferCount = 3`을 추가했다.

### 결정 사항과 이유
- **디버그 레이어는 디바이스 생성 전에 켠다** — `D3D12GetDebugInterface`→`EnableDebugLayer`를
  `D3D12CreateDevice`보다 먼저 호출하지 않으면 적용되지 않는다.
  DXGI 팩토리도 `DXGI_CREATE_FACTORY_DEBUG`로 만든다.
- **어댑터는 `IDXGIFactory6::EnumAdapterByGpuPreference`로 고른다** — 내장/외장 GPU가 함께 있는
  노트북에서 외장 GPU가 선택된다. 이 인터페이스가 없는 환경을 위해 `EnumAdapters1` 대비책도 둔다.
  WARP(소프트웨어 래스터라이저)는 건너뛴다.
- **`D3D12CreateDevice`의 마지막 인자에 `nullptr`을 넘겨 지원 여부만 검사** — 실제 디바이스를
  만들지 않고 후보를 걸러낼 수 있다.
- **기능 레벨은 생성 후 `CheckFeatureSupport`로 조회** — 최소 11_0으로 만들고 실제 상한을 따로 얻는다.
  `D3D_FEATURE_LEVEL_12_2`는 최신 SDK에만 있어서 목록에서 제외했다.
- **`ID3D12InfoQueue`로 오류/경고 시 디버거 중단** — 원인 추적이 훨씬 쉬워진다.
  단, 클리어 값 불일치 경고는 최적화 힌트일 뿐이라 필터로 걸러냈다.
- **디스크립터 칸 크기는 반드시 런타임 조회** — `GetDescriptorHandleIncrementSize`의 값은
  하드웨어마다 다르다. 상수로 박아두면 다른 GPU에서 깨진다. 타입별로 캐시해 둔다.
- **펜스 값은 1부터 시작** — 0을 "아직 제출된 적 없음"으로 쓰기 위해서다.
- **`Flush()`는 리소스 해제 직전에 필수** — GPU가 참조 중인 리소스를 해제하면 크래시가 난다.

---

## 3단계 — 스왑체인 / 렌더 타겟 / 깊이 버퍼, 그리고 프레임 루프

### 목표
화면이 실제로 지워지는(clear) 것까지 만든다. `Renderer`와 `Application`도 이 단계에서
함께 도입했다 — 그러지 않으면 `Main.cpp`에 임시 프레임 루프를 썼다가 다음 단계에서 다시
걷어내야 해서 불필요한 왕복이 생기기 때문이다. 이 단계의 `Renderer`는 화면 클리어까지만
담당하고, 파이프라인과 그리기는 4~5단계에서 얹는다.

### 추가한 파일
| 파일 | 역할 |
|---|---|
| `Graphics/D3D12Helpers.h` | 힙 속성/리소스 설명자/상태 전이 배리어 생성 헬퍼, 256B 정렬 |
| `Graphics/SwapChain.h/.cpp` | 스왑체인, 백버퍼 RTV, 깊이/스텐실 버퍼, 뷰포트, 리사이즈 |
| `Graphics/Renderer.h/.cpp` | 프레임 자원, 커맨드 리스트, `BeginFrame`/`EndFrame` |
| `Core/Application.h/.cpp` | 수명주기와 메인 루프. 창과 렌더러를 연결 |

### 결정 사항과 이유
- **`d3dx12.h`를 쓰지 않는다** — 흔히 쓰이지만 Windows SDK에 들어 있지 않아 별도로 받아야 한다.
  외부 의존성 없이 가려고 필요한 헬퍼만 `D3D12Helpers.h`에 직접 만들었다.
- **`CreateSwapChainForHwnd`에 디바이스가 아니라 커맨드 큐를 넘긴다** — D3D12에서 Present는
  큐를 통해 일어난다. D3D11과 가장 헷갈리는 지점이다.
- **`DXGI_SWAP_EFFECT_FLIP_DISCARD` + 백버퍼 3장** — 플립 모델은 필수에 가깝다.
- **`MakeWindowAssociation(DXGI_MWA_NO_ALT_ENTER)`** — DXGI가 Alt+Enter로 멋대로 전체 화면
  전환을 하면 스왑체인 상태가 꼬인다. 막아 둔다.
- **`ResizeBuffers` 전에 백버퍼 참조를 모두 해제** — 참조가 하나라도 남으면 실패한다.
  또한 GPU가 이전 백버퍼를 쓰고 있을 수 있으므로 그 전에 `Flush()`가 반드시 선행되어야 한다.
- **커맨드 얼로케이터는 백버퍼 장수만큼, 커맨드 리스트는 1개** — 얼로케이터는 GPU가 다 쓰기
  전에는 Reset할 수 없으므로 돌려쓸 개수가 필요하다. 반면 기록은 CPU 한 스레드에서만 하므로
  리스트는 하나로 충분하고, 매 프레임 얼로케이터만 바꿔 Reset한다.
- **백버퍼 상태 전이 PRESENT ↔ RENDER_TARGET** — D3D11에는 없던 개념이다. 빠뜨리면 디버그
  레이어가 즉시 오류를 낸다.

---

## 4단계 — 파이프라인 자원 (Shader / RootSignature / PSO / 버퍼 / 지오메트리)

### 목표
그리기에 필요한 재료를 모두 갖춘다.

### 추가한 파일
| 파일 | 역할 |
|---|---|
| `Graphics/Vertex.h` | 정점 구조체와 입력 레이아웃 |
| `Graphics/ConstantBuffers.h` | `ObjectConstants`(b0) / `PassConstants`(b1) |
| `Graphics/UploadBuffer.h` | 업로드 힙 템플릿 버퍼 (상수 버퍼 256B 정렬) |
| `Graphics/Shader.h/.cpp` | `D3DCompileFromFile` 런타임 컴파일 + 오류/경고 로깅 |
| `Graphics/RootSignature.h/.cpp` | 루트 시그니처 생성, 기본 구성(b0/b1) 제공 |
| `Graphics/PipelineState.h/.cpp` | PSO 생성, 기본값이 채워진 설명자 제공 |
| `Graphics/Mesh.h/.cpp` | 디폴트 힙 정점/인덱스 버퍼 + 업로드 경유 복사 |
| `Graphics/GeometryGenerator.h/.cpp` | 큐브 정점/인덱스 데이터 생성 (이때는 이름이 `GeometryFactory`였고, 9단계에서 지금 이름으로 바뀐다) |

### 결정 사항과 이유
- **디스크립터 테이블 대신 루트 CBV 2개** — 텍스처가 없는 단계에서는 셰이더 가시 디스크립터
  힙을 아예 만들 필요가 없어진다. 텍스처를 붙일 때 디스크립터 테이블을 추가하면 된다.
- **행렬은 CPU에서 전치해 올린다** — `XMMATRIX`는 행 우선 저장, HLSL 상수 버퍼는 열 우선
  해석이라 두 규약이 서로 상쇄된다. 곱셈은 행벡터 규약인 `mul(float4(pos,1), gWorld)` 순서를 쓴다.
- **정점/인덱스는 디폴트 힙** — 한 번 올리고 계속 읽기만 하므로 GPU 전용 메모리가 맞다.
  임시 업로드 버퍼를 거쳐 `CopyBufferRegion`으로 옮기고, **임시 버퍼는 GPU 복사가 끝날 때까지
  살아 있어야 하므로** `DisposeUploaders()`를 Flush 이후에 따로 호출한다.
- **`SampleMask = UINT_MAX`** — 0으로 두면 아무것도 그려지지 않는 흔한 실수가 된다.
- **큐브는 정점 24개** — 면마다 법선이 다르므로 8개를 공유하면 면 경계가 뭉개진다.

---

## 5단계 — 셰이더 / 카메라 / 그리기 경로 조립 (회전하는 큐브)

### 목표
4단계까지 만든 재료를 조립해 실제로 화면에 물체가 나오게 한다.

### 추가한 파일
| 파일 | 역할 |
|---|---|
| `Shaders/Common.hlsli` | VS/PS 공용 cbuffer와 구조체 정의 |
| `Shaders/Basic_VS.hlsl` | 월드 → 클립 공간 변환, 조명용 값 전달 |
| `Shaders/Basic_PS.hlsl` | 방향광 1개 Blinn-Phong 조명 |
| `Graphics/Camera.h/.cpp` | 이때는 `LookAt`만 있는 단순 카메라. 8단계에서 1인칭 카메라로 확장된다 |
| `Graphics/Renderer.h/.cpp` | 루트 시그니처·PSO 생성, `SetPassConstants`/`DrawMesh` 추가 |
| `Core/Application.h/.cpp` | 큐브 생성, 회전 갱신, 그리기 호출 |

### 결정 사항과 이유
- **상수 버퍼를 프레임마다 따로 둔다** — 공유하면 GPU가 아직 읽는 중인 값을 CPU가 다음
  프레임에 덮어써 버린다.
- **법선 행렬은 월드의 역전치** — 비균등 스케일에서도 법선이 표면에 수직으로 남는다.
- **`gLightDirection`은 빛이 나아가는 방향** — 셰이더에서 부호를 뒤집어 쓴다.
- **루트 시그니처는 매 프레임 다시 설정** — 커맨드 리스트를 Reset하면 지워진다.
- **`FXMMATRIX` 대신 `const XMMATRIX&`** — `__vectorcall` 사용 여부에 따른 호출 규약 차이를
  일반 멤버 함수에서 피하기 위해서다.
- **셰이더는 `None` 항목으로 등록** — 런타임 컴파일이므로 MSBuild가 빌드하지 않아야 한다.

---

## 빌드 수정 — `D3D12_PRIMITIVE_TOPOLOGY_TRIANGLELIST` 미선언 오류

Visual Studio 첫 빌드에서 나온 오류.

```
Renderer.cpp: error C2065:
  'D3D12_PRIMITIVE_TOPOLOGY_TRIANGLELIST': 선언되지 않은 식별자입니다.
```

`IASetPrimitiveTopology`가 받는 `D3D12_PRIMITIVE_TOPOLOGY`는 **`D3D_PRIMITIVE_TOPOLOGY`의
typedef**이므로 열거값 접두사가 `D3D12_`가 아니라 `D3D_`다. 이름이 비슷한 두 열거형을
혼동한 것이다.

| 식별자 | 소속 | 쓰는 곳 |
|---|---|---|
| `D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE` | `D3D12_PRIMITIVE_TOPOLOGY_TYPE` | PSO의 `PrimitiveTopologyType` (삼각형 "부류") |
| `D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST` | `D3D_PRIMITIVE_TOPOLOGY` | 커맨드 리스트의 `IASetPrimitiveTopology` (리스트/스트립 구분) |

`Renderer.cpp`를 `D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST`로 고치고, 함께 `Core/stdafx.h`에
전이 include로 우연히 딸려 오던 표준 헤더(`<cstring>` `std::memcpy`, `<cstdio>`
`swprintf_s`/`_vsnwprintf_s`, `<climits>` `UINT_MAX`)를 명시적으로 추가했다.

---

## 6단계 — 입력 시스템 (InputReader) 과 1인칭 카메라

### 목표
WASD로 표준적인 앞뒤좌우 이동, 마우스 우클릭 드래그로 카메라 회전.

### 추가·수정한 파일
| 파일 | 역할 |
|---|---|
| `Input/InputReader.h/.cpp` | 키보드/마우스 상태 수집, 프레임 단위 질의 (신규) |
| `Graphics/Camera.h/.cpp` | LookAt 전용 → yaw/pitch 기반 1인칭 카메라로 확장 |
| `Core/Window.h/.cpp` | 원시 메시지 콜백(`SetMessageCallback`) 추가, 키 콜백 제거 |
| `Core/Application.h/.cpp` | 입력 배선, `UpdateCamera` 추가 |

### 결정 사항과 이유

**`Window`는 입력의 의미를 모른다.** 받은 메시지를 `SetMessageCallback`으로 그대로
흘려보내고, 해석은 `InputReader`가 한다. `Window`가 D3D를 모르는 것과 같은 이유다.
ESC로 창을 닫는 것만 창의 책임으로 남겼다.

**`BeginFrame`은 메시지 펌프보다 먼저 부른다.** 프레임 시작에 "이전 프레임 상태 = 현재
상태"로 보관해 두고, 그 뒤에 들어오는 메시지가 현재 상태를 갱신한다. 그래야
`WasKeyPressed`(= 현재 눌림 && 이전엔 안 눌림)가 정확히 한 프레임만 참이 된다.

```cpp
m_input->BeginFrame();          // 이전 프레임 상태 보관, 마우스 이동량 0으로 초기화
m_window->ProcessMessages();    // 이 사이에 ProcessMessage가 여러 번 불린다
// 이후 IsKeyDown / WasKeyPressed / GetMouseDeltaX ... 질의
```

**토글은 `WasKeyPressed`, 이동은 `IsKeyDown`.** V(수직동기화)·Space(회전정지)를
`IsKeyDown`으로 처리하면 키를 누르고 있는 동안 매 프레임 뒤집혀서 사실상 동작하지 않는다.

**이동은 시간에 비례, 회전은 픽셀 수에만 비례.** 이동에 `deltaTime`을 곱하는 것은
프레임률과 무관한 속도를 위해서다. 반대로 마우스 회전에 `deltaTime`을 곱하면 **안 된다**.
이동량 자체가 이미 이번 프레임 동안 움직인 픽셀 수라, 여기에 또 시간을 곱하면 프레임률에
따라 감도가 달라진다.

**우클릭 시 마우스를 캡처한다.** `SetCapture`를 하면 드래그가 창 밖으로 나가도 이동량이
계속 들어온다. 함께 커서를 숨긴다(`ShowCursor`는 참조 카운트라 호출 짝을 반드시 맞춰야
한다). `WM_CAPTURECHANGED`/`WM_KILLFOCUS`도 처리해 캡처·커서·눌린 키가 어긋난 채 남지
않게 했다.

**카메라 회전을 기저 벡터가 아니라 yaw/pitch 각도로 들고 있는다.** right/up/look을 매
프레임 조금씩 회전시키는 방식은 부동소수점 오차가 쌓이면서 화면이 서서히 기울어진다(roll).
각도를 누적하고 매번 처음부터 벡터를 새로 만들면 그런 누적 오차가 없고, 위아래 시야
제한도 `std::clamp` 한 줄로 끝난다. 수직(90도)에 정확히 닿으면 `cross(worldUp, look)`이
무너지므로 89도에서 멈춘다.

**`Fly`는 카메라 up이 아니라 월드 up으로 움직인다.** 위를 올려다보는 중에도 상승 방향이
흔들리지 않아 조작이 예측 가능해진다.

### 카메라 수식 검증
`RebuildBasis`는 `(0,0,1)`에 pitch(X축) → yaw(Y축) 순 회전을 적용하므로
`look = (cos p · sin y, −sin p, cos p · cos y)`이고, `LookAt`은 이를 역산한다.
파이썬으로 수치 검증했다 — LookAt 왕복(방향→각도→방향) 오차 `1.1e-16`, 정면/우향
90도/아래 30도 각각에서 look·right·up 직교성 확인, 오른쪽 드래그 시 우회전·아래
드래그 시 아래를 보는 부호 확인.

### 조작 (이 단계에서 확정된 것)
| 입력 | 동작 |
|---|---|
| `W` `A` `S` `D` | 앞 / 왼쪽 / 뒤 / 오른쪽 이동 |
| `Q` `E` | 아래 / 위로 이동 |
| `Shift` (누른 채) | 이동 속도 3배 |
| 마우스 우클릭 + 드래그 | 카메라 시야 회전 |
| `Space` | 큐브 회전 정지 / 재개 |
| `V` | 수직 동기화 토글 |
| `ESC` | 종료 |

---

## 7단계 — 지오메트리 생성과 메시 관리 분리 (GeometryGenerator + MeshFactory)

### 목표
격자(grid) 메시를 추가하고 바닥 평면을 놓되, 하드코딩이 아니라 팩토리를 통해 만들고
관리하는 구조로 간다.

### 추가·수정한 파일
| 파일 | 내용 |
|---|---|
| `Graphics/GeometryGenerator.h/.cpp` | `GeometryFactory`에서 이름 변경. `CreateGrid` / `CreatePlane` 추가 |
| `Graphics/MeshFactory.h/.cpp` | 메시 생성·캐시·수명 관리 클래스 (신규) |
| `Core/Application.h/.cpp` | 메시를 팩토리 경유로 획득, 바닥 격자 추가 |

### 결정 사항과 이유

**책임을 두 층으로 나눴다.** `GeometryGenerator`(네임스페이스)는 순수한 CPU 데이터만
계산하고, `MeshFactory`(클래스)가 그것을 GPU에 올려 이름으로 관리한다. 도형 계산을 GPU
없이 단독으로 시험할 수 있고, 새 도형을 넣을 때 렌더링 코드를 건드리지 않는다.

**팩토리가 메시를 소유하고, 호출부는 관찰 포인터만 받는다.** 직접 만들면 도형 계산·GPU
업로드(제출→대기→임시 버퍼 해제)·소멸 시점 맞추기가 호출부에 흩어진다. 팩토리가 셋을
한곳에서 처리한다.

**`unordered_map<wstring, unique_ptr<Mesh>>`로 담는다.** `Mesh`를 값으로 담으면 맵이
재해싱될 때 원소가 이사하면서 이미 나눠 준 `Mesh*`가 전부 무효가 된다. `unique_ptr`이면
주소가 고정된다.

**같은 이름은 재사용한다.** 여러 오브젝트가 같은 도형을 공유할 때 중복 업로드와 GPU 메모리
낭비를 막는다.

> 이 단계에서는 `CreateGrid`가 칸마다 다른 색을 정점에 구워서 체커 무늬를 만들고,
> 그러기 위해 칸마다 정점을 4개씩 따로 두었다(정점을 공유하면 색이 경계에서 섞이므로).
> 이 방식은 9단계에서 바닥이 멀리서 어른거리는 문제의 원인 중 하나로 다시 다뤄지고,
> 결국 색을 굽지 않는 방식으로 바뀐다.

**`uint16_t` 인덱스 한계를 런타임에 처리한다.** 넘으면 실패시키지 않고 가로세로 비율을
유지한 채 줄이고 경고를 남긴다.

---

## 8단계 — DirectXProj(DirectX 11) 제거 및 솔루션·문서 정리

### 배경
저장소에 DX11 프로젝트와 DX12 프로젝트가 함께 있었으나, 앞으로의 작업은 전부 DX12로
진행하기로 해 `DirectXProj`를 저장소에서 제거했다. DX12Framework는 처음부터 별도
프로젝트로 만들었고 DirectXProj를 코드로 참조한 적이 없어, 삭제로 인한 코드 변경은 없었다.

### 함께 정리한 것
| 대상 | 내용 |
|---|---|
| 솔루션 파일 | `DirectXProj` 프로젝트 등록/구성 매핑 제거, `DirectXProj.sln` → `DX12Framework.sln`으로 이름 변경 |
| 루트 `README.md` | DX12Framework 기준으로 새로 작성 |

`DX12Framework/.editorconfig`는 원래 DirectXProj에서 복사해 온 파일이지만 이미 독립된
사본이라 삭제의 영향을 받지 않는다.

---

## 9단계 — 4x MSAA 도입 및 Tab 와이어프레임 토글

### 배경
체커 무늬 바닥에서 카메라를 움직이면 회색 칸 경계가 번지는 현상이 있었다.
`SwapChain::CreateSwapChain`에서 `SampleDesc.Count = 1`로 안티앨리어싱이 전혀 없었던
탓으로, 고대비 체커보드가 겪는 전형적인 지오메트리 앨리어싱/모아레였다. 4x MSAA로
해결을 시도했다(완전한 해결은 10단계에서 이루어진다). 같은 작업에서 Tab 키로
솔리드/와이어프레임을 토글하는 기능도 함께 추가했다.

### 추가·수정한 파일
| 파일 | 내용 |
|---|---|
| `Graphics/D3D12Device.h/.cpp` | `QueryMsaaQualityLevels` 추가 — 포맷+샘플수 지원 여부/품질 레벨 조회 |
| `Graphics/SwapChain.h/.cpp` | MSAA 색상 타겟 + 전용 RTV 힙, 깊이 버퍼 샘플 수 동기화, `GetColorTargetView`/`IsMsaaEnabled` |
| `Graphics/Renderer.h/.cpp` | PSO를 솔리드/와이어프레임 2개로, `BeginFrame`/`EndFrame`에 MSAA 분기와 Resolve 추가 |
| `Core/Application.h/.cpp` | Tab 토글 처리 (`m_wireframeEnabled`) |

### 결정 사항과 이유

**MSAA 색상 타겟을 스왑체인과 별도로 둔다.** 플립 모델(`DXGI_SWAP_EFFECT_FLIP_DISCARD`)
백버퍼는 멀티샘플을 직접 지원하지 않는다. 하드웨어가 지원하면 별도의 멀티샘플 렌더
타겟(`m_msaaColorTarget`)에 그린 뒤, `EndFrame`에서 `ResolveSubresource`로 그 내용을
백버퍼에 내려받는다(다운샘플). 지원하지 않는 하드웨어에서는 예전처럼 백버퍼에 직접
그린다 — 두 경로의 차이는 `SwapChain::GetColorTargetView()`가 감춰서 `Renderer`가
최소한만 분기하면 된다.

**MSAA 지원 여부는 런타임에 조회하고, 안 되면 자동으로 끈다.**
`CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS)`로 4x MSAA 지원을 확인해
`NumQualityLevels`가 0이면(미지원) 샘플 수를 1로 낮추고 경고 로그만 남긴다.

**깊이 버퍼도 색상 타겟과 반드시 같은 샘플 수를 가져야 한다.** 다르면 디바이스 제거로
이어진다. 멀티샘플 텍스처는 DSV 차원도 `D3D12_DSV_DIMENSION_TEXTURE2DMS`로 만들어야 하며,
이 경우 밉 슬라이스 개념이 없다는 점도 분기 처리했다.

**리소스 상태는 프레임 경계마다 항상 대칭으로 되돌린다.** MSAA 타겟은 매 프레임
`RENDER_TARGET`으로 시작해 `RENDER_TARGET`으로 끝나고, 백버퍼는 `PRESENT`로 시작해
`PRESENT`로 끝난다. `BeginFrame`이 매번 같은 시작 상태를 가정할 수 있게 했다.

**와이어프레임은 PSO를 두 개 만들어 두고 골라 쓴다.** PSO는 불변 객체라 "지금 프레임만
채우기 모드를 바꾼다"가 불가능하다. `FillMode`만 다르고 나머지는 완전히 같은 PSO를
초기화 시점에 함께 만들어 두고, `BeginFrame`에서 `m_wireframeEnabled` 값에 따라
`m_commandList->Reset(allocator, pso)`에 넘길 PSO만 고른다. 같은 셰이더를 그대로 쓰므로
와이어프레임 선에도 조명이 적용된 색이 나온다.

**와이어프레임 상태는 `Renderer`가 들고 있는다.** `SetWireframe(bool)`을 추가했는데,
위치는 기존에 `SetDirectionalLight`로 조명값을 들고 있던 것과 같은 자리다. "가끔
바뀌고 여러 프레임에 걸쳐 유지되는 렌더 상태"는 `Renderer` 멤버로 두는 기존 관례를
따랐다.

### 이 단계에서 아직 남아 있던 문제
4x MSAA를 적용해도 카메라를 움직이면 바닥 격자가 (정도는 약하지만) 여전히 번졌다.
원인은 10단계에서 다시 규명한다.

---

## 10단계 — 바닥 그리드 잔여 앨리어싱의 근본 원인 해결 (절차적 필터링 체커)

### 배경
9단계의 4x MSAA 적용 후에도 바닥 격자 경계가 남아서 번지는 현상이 있었다. 원인을 다시
파고들었다.

**`kFrameBufferCount`(트리플 버퍼링)나 프레임 리소스 개수는 이 현상과 무관하다.** 그건
CPU/GPU 동기화(스톨) 문제이지 — 부족하면 프레임이 끊기거나 fps가 떨어지는 식으로
나타난다 — 화면에 그려지는 픽셀 값이 잘못되는 문제가 아니다.

**진짜 원인은 MSAA의 근본적인 한계였다.** MSAA는 "한 프레임 안에서 삼각형과 삼각형의
경계"를 정해진 샘플 개수(4개)로만 안티앨리어싱한다. 20×20 격자는 카메라에서 먼 곳
(수평선 근처)에서 칸 하나가 화면 1픽셀보다 작아지는데, 그 정도로 축소되면 픽셀
하나에 여러 칸의 경계가 들어와도 고정된 4샘플로는 다 담아내지 못한다. 카메라가 아주
조금만 움직여도 그 몇 안 되는 샘플이 어느 칸을 맞히는지가 프레임마다 바뀌며
반짝인다(shimmer) — MSAA 샘플 수를 늘려도 정도만 줄어들 뿐 근본적으로 없어지지
않는, 지오메트리/셰이딩 축소(minification) 앨리어싱이다.

### 해결: 체커를 정점에 굽지 않고 픽셀 셰이더에서 필터링해 계산
Inigo Quilez의 "filterable checkerboard" 기법을 적용했다. 픽셀 셰이더에서 `ddx`/`ddy`로
월드 좌표가 화면 1픽셀당 얼마나 변하는지(도함수)를 구하고, 그 폭만큼 체커 패턴을
박스 필터로 미리 적분해 흐린다. 무늬가 화면에서 작아질수록 필터 폭이 넓어져 값이
자연스럽게 0.5(중간 회색)로 수렴하므로, 카메라가 움직여도 어른거리지 않는다.

파이썬으로 수식을 검증했다 — 도함수가 작을 때(가까이서) 하드 체커와 오차 `5.5e-13`,
도함수가 클 때(멀리서) 정확히 `0.5`로 수렴, 값 범위는 항상 `[0, 1]`.

### 추가·수정한 파일
| 파일 | 내용 |
|---|---|
| `Graphics/ConstantBuffers.h`, `Shaders/Common.hlsli` | `ObjectConstants`(b0)에 `checkerColorB`/`checkerCellSize` 추가 (176바이트, 16B 정렬) |
| `Shaders/Basic_PS.hlsl` | `FilteredChecker()` 추가. `checkerCellSize > 0`일 때만 적용되고, 0(기본값)이면 기존처럼 정점 색을 그대로 쓴다 |
| `Graphics/Renderer.h/.cpp` | `DrawMesh`에 `checkerColorB`/`checkerCellSize` 매개변수 추가 (기본값 0 = 기존 동작과 동일) |
| `Graphics/GeometryGenerator.h/.cpp` | `CreateGrid`/`CreatePlane`에서 색 매개변수 제거, 공유 정점 방식으로 재작성 |
| `Graphics/MeshFactory.h/.cpp` | `CreateGrid`/`CreatePlane` 시그니처에서 색 매개변수 제거 |
| `Core/Application.h/.cpp` | 바닥 색/칸 크기를 멤버(`m_groundColorA`/`m_groundColorB`/`m_groundCellSize`)로 보관하고 `Render()`의 `DrawMesh` 호출에서 넘김 |

### 곁가지 개선: 격자를 정점 공유 방식으로 재작성
7단계에서는 체커 색을 정점에 구워야 한다는 이유로 칸마다 정점을 4개씩 따로 두고
있었다. 색을 셰이딩 단계로 옮기면서 그 이유 자체가 없어졌다. 평면은 법선과 색이 모두
같으므로 이제 `(rowCount+1) x (columnCount+1)`개의 정점을 공유하는 표준적인 격자로
다시 만들었다(20×20 기준 정점 1600 → 441개, 74% 감소). 감기 순서는 기존에 검증된
큐브 윗면과 동일하게 `(x-,z-)→(x-,z+)→(x+,z+)→(x+,z-)`로 맞췄다. 인덱스 한계도
"칸 수" 대신 정확한 "정점 수" 기준(`kMaxGridVertices = 65536`)으로 다시 계산한다.

`GeometryGenerator`가 색을 다루지 않게 된 것은 "지오메트리는 순수 CPU 데이터만
다루고, 셰이딩(색/무늬)은 렌더러의 책임"이라는 7단계 설계 원칙을 원래 형태로 되돌린
것이기도 하다 — 체커 색을 정점에 구운 것 자체가 그 원칙에서 벗어난 임시방편이었다.

### 검증
- `FilteredChecker` 수식을 넘파이로 검증 (수렴성/범위/하드 체커와의 일치, 위 참고)
- 공유 정점 격자의 감기 순서를 기존 큐브 윗면과 좌표 단위로 대조 (일치)
- 정점 수 상한 시뮬레이션: 20×20 → 정점 441, 300×300 요청 → 255×255로 축소되며
  최대 인덱스 정확히 65535
- `ObjectConstants`(C++) ↔ `cbObject`(HLSL) 필드 크기/순서/총 바이트(176B) 일치,
  16B 정렬 확인

---

## 현재 상태 정리

### 완성된 것
디바이스 · 커맨드 큐 · 펜스 · 스왑체인(+4x MSAA) · 디스크립터 힙 · 루트 시그니처 ·
PSO(솔리드/와이어프레임) · 리소스 상태 전이 · 업로드/디폴트 힙 구분 · 프레임 자원
다중화까지 DX12의 기본기가 들어간 프레임워크. 체커 무늬 바닥(절차적, 앤티앨리어싱됨)
위에서 회전하는 큐브를 WASD + 마우스로 돌아다니며 볼 수 있고, Tab으로 와이어프레임을
토글할 수 있다.

의존 방향은 `Core → (없음)`, `Graphics → Core`, `Input → Core`,
`Application → Core + Graphics + Input` 한 방향으로 유지했다. 외부 라이브러리
의존성은 없다.

### 현재 조작
| 입력 | 동작 |
|---|---|
| `W` `A` `S` `D` | 앞 / 왼쪽 / 뒤 / 오른쪽 이동 |
| `Q` `E` | 아래 / 위로 이동 |
| `Shift` (누른 채) | 이동 속도 3배 |
| 마우스 우클릭 + 드래그 | 카메라 시야 회전 |
| `Space` | 큐브 회전 정지 / 재개 |
| `Tab` | 솔리드 / 와이어프레임 토글 |
| `V` | 수직 동기화 토글 |
| `ESC` | 종료 |

### 검증 상태 (중요)
작업 환경이 Linux 컨테이너라 **MSVC/Windows SDK가 없어 실제 빌드로 검증하지 못했다.**
각 단계마다 다음을 스크립트로 대조하며 작성했다.

- 상수 버퍼 C++ 구조체 ↔ HLSL cbuffer: 필드 크기·순서·총 바이트 일치, 16B 정렬
- 정점 입력 레이아웃 ↔ HLSL 시맨틱/포맷 일치
- 헤더 선언 대비 정의 누락 여부 (매 단계 0건 유지)
- `.vcxproj` / `.filters` 파일 목록과 디스크 파일 1:1 일치
- 카메라 회전 행렬, 격자 감기 순서, 체커 필터링 수식 등 핵심 수학은 파이썬으로 수치 검증
- 전 파일 중괄호/소괄호 균형

**Visual Studio에서 실제로 빌드해 봐야 최종 확인이 된다.** 첫 빌드에서 오류가 나면
`DX12Framework.log`의 마지막 줄을 보면 어느 단계에서 멈췄는지 바로 알 수 있다.

### 다음에 붙이면 좋을 것
1. **텍스처** — `DescriptorHeap`을 `CBV_SRV_UAV` + `shaderVisible=true`로 만들고
   루트 시그니처에 디스크립터 테이블과 정적 샘플러를 추가한다. 이미 그 확장을 염두에
   두고 설계했다.
2. **오브젝트 여러 개** — `Application`이 목록을 들고 `DrawMesh`를 반복 호출한다.
   프레임당 상한은 `kMaxObjectsPerFrame`(256).
3. **GameObject-Component 계층** — Scene / GameObject / Component / Transform /
   MeshRenderer를 이 렌더링 코어 위에 얹는다.
4. **반투명 렌더링** — 블렌딩을 켠 PSO를 하나 더 만들고 그리기 순서를 분리한다.
5. **도형 추가** (구, 원기둥 등) — `GeometryGenerator`에 함수 하나를 추가하고
   `MeshFactory::Create`로 등록한다.
