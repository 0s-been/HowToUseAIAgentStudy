# DirectX 12 프레임워크 작업 로그

기존 `DirectXProj`(DirectX 11)는 그대로 두고, 같은 솔루션에 `DX12Framework` 프로젝트를 새로 추가해
DirectX 12 기본 프레임워크를 단계적으로 구축한 기록이다.

- 대상 솔루션: `DirectXProj.sln`
- 신규 프로젝트: `DX12Framework/DX12Framework.vcxproj` (v143 / C++20 / x64·Win32)
- 셰이더 처리 방식: `D3DCompileFromFile`을 이용한 **런타임 컴파일**
- 작업 브랜치: `claude/directx12-framework-setup-g32hsz`

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
| `Main.cpp` | `wWinMain` 진입점, 임시 메시지 루프 |
| `DX12Framework.vcxproj` / `.filters` | 프로젝트 정의 |
| `.editorconfig` | `DirectXProj`와 동일한 코드 스타일(탭 4칸) |

### 결정 사항과 이유
- **PCH 미사용** — `stdafx.h`는 단순 공통 헤더로만 쓴다. 빌드 설정이 단순해지고
  파일을 추가할 때 실수할 여지가 줄어든다.
- **`/utf-8` 컴파일 옵션** — 소스에 한글 주석이 있어 소스/실행 문자 집합을 UTF-8로 고정했다.
- **`Window`는 D3D를 전혀 모른다** — 창 크기 변경을 `std::function` 콜백으로만 알린다.
  덕분에 창 코드와 그래픽 코드를 서로 독립적으로 수정할 수 있다.
- **리사이즈는 드래그가 끝날 때 한 번만** — 테두리를 드래그하는 동안에는 `WM_SIZE`가 수십 번 발생한다.
  매번 스왑체인을 재생성하면 매우 느려지므로, `WM_ENTERSIZEMOVE`/`WM_EXITSIZEMOVE`로 구간을 잡아
  드래그가 끝난 시점에 한 번만 콜백을 호출한다.
- **로그 매크로에 서식 문자열까지 `__VA_ARGS__`로 넘김** — `__VA_ARGS__`가 절대 비지 않게 되어
  MSVC의 기존 전처리기와 `/Zc:preprocessor` 양쪽에서 모두 동작한다.
- **로그 파일에 UTF-8 BOM 기록** — 메모장으로 열어도 한글이 깨지지 않는다.

### 이 단계의 확인 방법
`Debug|x64`로 빌드 후 실행 → 창이 뜨고, 타이틀바에 FPS가 갱신되며,
실행 폴더에 `DX12Framework.log`가 생성된다. ESC로 종료된다.

---

## 2단계 — 디바이스 계층 (Device / CommandQueue / DescriptorHeap)

### 목표
D3D11에서는 `D3D11CreateDeviceAndSwapChain` 한 번이면 끝나던 초기화가,
D3D12에서는 디바이스·커맨드 큐·스왑체인으로 완전히 쪼개져 있다.
그중 화면과 무관한 "디바이스" 부분을 먼저 세운다.

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
  덕분에 `WaitForFenceValue(0)`이 즉시 반환되어 첫 프레임 처리가 단순해진다.
- **`Flush()`는 리소스 해제 직전에 필수** — GPU가 참조 중인 리소스를 해제하면 크래시가 난다.
  `CommandQueue::Shutdown`에서도 자동으로 한 번 호출한다.
- **RTV/DSV 힙에 `shaderVisible=true`가 들어오면 강제로 false로 교정** — D3D12 규칙상 불가능한
  조합이라 디바이스 제거로 이어진다. 경고를 남기고 바로잡는다.

### 이 단계의 확인 방법
실행하면 로그에 선택된 어댑터 이름, 기능 레벨, 타입별 디스크립터 칸 크기,
티어링 지원 여부가 남는다. 종료 시 live object 보고에 누수가 없어야 한다.

---

## 3단계 — 스왑체인 / 렌더 타겟 / 깊이 버퍼, 그리고 프레임 루프

### 목표
화면이 실제로 지워지는(clear) 것까지 만든다. 여기까지 오면 "DX12가 돌아간다"를 눈으로 확인할 수 있다.

### 계획 대비 변경
원래 계획에서는 `Renderer`와 `Application`을 5단계에 두려 했으나, 3단계에서 도입하는 쪽으로 앞당겼다.
그렇지 않으면 `Main.cpp`에 프레임 루프용 임시 코드를 썼다가 5단계에서 다시 걷어내야 해서
불필요한 왕복이 생긴다. 대신 이 단계의 `Renderer`는 화면 클리어까지만 담당하고,
파이프라인과 그리기는 다음 단계에서 얹는다.

### 추가한 파일
| 파일 | 역할 |
|---|---|
| `Graphics/D3D12Helpers.h` | 힙 속성/리소스 설명자/상태 전이 배리어 생성 헬퍼, 256B 정렬 |
| `Graphics/SwapChain.h/.cpp` | 스왑체인, 백버퍼 RTV, 깊이/스텐실 버퍼, 뷰포트, 리사이즈 |
| `Graphics/Renderer.h/.cpp` | 프레임 자원, 커맨드 리스트, `BeginFrame`/`EndFrame` |
| `Core/Application.h/.cpp` | 수명주기와 메인 루프. 창과 렌더러를 연결 |

`Main.cpp`는 `Application`을 만들고 실행하는 20줄짜리로 정리했다.

### 결정 사항과 이유
- **`d3dx12.h`를 쓰지 않는다** — 흔히 쓰이지만 Windows SDK에 들어 있지 않아 별도로 받아야 한다.
  외부 의존성 없이 가려고 필요한 헬퍼만 `D3D12Helpers.h`에 직접 만들었다.
- **`CreateSwapChainForHwnd`에 디바이스가 아니라 커맨드 큐를 넘긴다** — D3D12에서 Present는
  큐를 통해 일어난다. D3D11과 가장 헷갈리는 지점이다.
- **`DXGI_SWAP_EFFECT_FLIP_DISCARD` + 백버퍼 3장** — 플립 모델은 필수에 가깝다.
  단, 플립 모델 백버퍼에는 MSAA를 직접 걸 수 없다. 필요하면 별도 렌더 타겟에 그린 뒤 Resolve해야 한다.
- **티어링 플래그는 스왑체인 생성 시점에 넣는다** — Present에서만 요청할 수 없다.
  그래서 `D3D12Device`가 미리 조사해 둔 티어링 지원 여부를 생성 시 반영한다.
- **`MakeWindowAssociation(DXGI_MWA_NO_ALT_ENTER)`** — DXGI가 Alt+Enter로 멋대로
  전체 화면 전환을 하면 스왑체인 상태가 꼬인다. 막아 둔다.
- **깊이 버퍼에 최적 클리어 값을 지정** — 지정하면 클리어가 빨라지지만,
  실제 `ClearDepthStencilView` 값과 다르면 디버그 레이어가 경고한다. 둘을 1.0f로 맞췄다.
- **`ResizeBuffers` 전에 백버퍼 참조를 모두 해제** — 참조가 하나라도 남으면 실패한다.
  또한 GPU가 이전 백버퍼를 쓰고 있을 수 있으므로 그 전에 `Flush()`가 반드시 선행되어야 한다.
- **커맨드 얼로케이터는 백버퍼 장수만큼, 커맨드 리스트는 1개** — 얼로케이터는 GPU가 다 쓰기 전에
  Reset할 수 없으므로 돌려쓸 개수가 필요하다. 반면 기록은 CPU 한 스레드에서만 하므로
  리스트는 하나로 충분하고, 매 프레임 얼로케이터만 바꿔 Reset한다.
- **`CreateCommandList` 직후 `Close()`** — 생성 시 '기록 중' 상태로 나오기 때문에,
  `BeginFrame`이 항상 `Reset`으로 시작하는 일관된 형태가 되도록 한 번 닫아 둔다.
- **뷰포트/가위는 매 프레임 다시 설정** — 커맨드 리스트를 Reset하면 초기화된다.
- **백버퍼 상태 전이 PRESENT ↔ RENDER_TARGET** — D3D11에는 없던 개념이다.
  빠뜨리면 디버그 레이어가 즉시 오류를 낸다.

### 이 단계의 확인 방법
실행하면 짙은 남색으로 지워진 창이 뜬다. 창 크기를 바꿔도 크래시 없이 종횡비가 유지되고,
로그에 리사이즈 기록이 남는다. V키로 수직 동기화를 토글하면 타이틀바의 fps가 달라진다.

---

## 4단계 — 파이프라인 자원 (Shader / RootSignature / PSO / 버퍼 / 지오메트리)

### 목표
그리기에 필요한 재료를 모두 갖춘다. 아직 실제로 그리지는 않고, 다음 단계에서 조립한다.

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
| `Graphics/GeometryFactory.h/.cpp` | 큐브 정점/인덱스 데이터 생성 |

`D3D12Helpers.h`에 래스터라이저/블렌드/깊이스텐실 기본값 함수를 추가했다.

### 결정 사항과 이유
- **디스크립터 테이블 대신 루트 CBV 2개** — 텍스처가 없는 단계에서는 셰이더 가시 디스크립터 힙을
  아예 만들 필요가 없어진다. `SetGraphicsRootConstantBufferView`에 GPU 주소만 넘기면 끝이라
  코드가 훨씬 단순하다. 텍스처를 붙일 때 디스크립터 테이블을 추가하면 된다.
- **행렬은 CPU에서 전치해 올린다** — `XMMATRIX`는 행 우선 저장, HLSL 상수 버퍼는 열 우선 해석이라
  두 규약이 서로 상쇄된다. 결과적으로 셰이더에서는 원래 행렬이 그대로 보이고,
  곱셈은 행벡터 규약인 `mul(float4(pos,1), gWorld)` 순서를 쓴다.
- **상수 버퍼 원소는 256바이트 정렬** — D3D12 규칙이다. `UploadBuffer`가 `isConstantBuffer`일 때
  자동으로 올림 처리한다.
- **업로드 버퍼는 Map을 풀지 않는다** — D3D12에서 권장되는 방식이다.
  Map/Unmap을 반복하는 쪽이 오히려 비용이 크다.
- **정점/인덱스는 디폴트 힙** — 한 번 올리고 계속 읽기만 하므로 GPU 전용 메모리가 맞다.
  CPU가 직접 쓸 수 없어 임시 업로드 버퍼를 거쳐 `CopyBufferRegion`으로 옮긴다.
  **임시 버퍼는 GPU 복사가 끝날 때까지 살아 있어야 하므로** `DisposeUploaders()`를
  Flush 이후에 따로 호출하도록 분리했다.
- **`SampleMask = UINT_MAX`** — 0으로 두면 아무것도 그려지지 않는다. 원인을 찾기 어려운 흔한 실수라
  기본 설명자에 명시적으로 넣었다.
- **루트 시그니처에 `ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT` 플래그** — 빠뜨리면 정점 버퍼가 무시된다.
- **큐브는 정점 24개** — 면마다 법선이 다르므로 8개를 공유하면 면 경계가 뭉개진다.
  여섯 면에 서로 다른 색을 주어 회전이 눈에 잘 보이게 했다.
- **인덱스는 `uint16_t`** — 인덱스 버퍼 뷰의 `DXGI_FORMAT_R16_UINT`와 반드시 짝을 맞춰야 한다.
- **셰이더 타깃은 `vs_5_1` / `ps_5_1`** — `D3DCompileFromFile`이 지원하는 상한이면서
  D3D12 루트 시그니처와 함께 쓰기에 충분하다.
- **셰이더 파일을 못 찾는 경우를 따로 로그** — 런타임 컴파일에서 가장 흔한 실패 원인이
  작업 디렉터리 설정이라, 그 경우를 구분해 안내 메시지를 남긴다.

---

## 5단계 — 셰이더 / 카메라 / 그리기 경로 조립 (회전하는 큐브)

### 목표
4단계까지 만든 재료를 조립해 실제로 화면에 물체가 나오게 한다.

### 추가·수정한 파일
| 파일 | 역할 |
|---|---|
| `Shaders/Common.hlsli` | VS/PS 공용 cbuffer와 구조체 정의 |
| `Shaders/Basic_VS.hlsl` | 월드 → 클립 공간 변환, 조명용 값 전달 |
| `Shaders/Basic_PS.hlsl` | 방향광 1개 Blinn-Phong 조명 |
| `Graphics/Camera.h/.cpp` | 왼손 좌표계 뷰/투영 행렬 |
| `Graphics/Renderer.h/.cpp` | 루트 시그니처·PSO 생성, `SetPassConstants`/`DrawMesh` 추가 |
| `Core/Application.h/.cpp` | 큐브 생성, 회전 갱신, 그리기 호출 |

### 결정 사항과 이유
- **상수 버퍼를 프레임마다 따로 둔다** — 하나를 공유하면 GPU가 아직 읽는 중인 값을
  CPU가 다음 프레임에 덮어써 버린다. 화면이 미묘하게 떨리는 원인이 되고 추적도 어렵다.
  `FrameContext`마다 `objectCB`(오브젝트 256개분)와 `passCB`(1개)를 둔다.
- **`StoreForShader` 헬퍼로 전치를 한 곳에 모았다** — "HLSL이 행렬 X를 보게 하려면 transpose(X)를
  올린다"는 규칙을 함수 하나로 캡슐화해, 호출부에서는 수학적으로 원하는 행렬만 넘기면 된다.
- **법선 행렬은 월드의 역전치** — 행벡터 규약에서 `n' = n * (W⁻¹)ᵀ`이다.
  균등 스케일만 쓸 거라면 월드 행렬로도 충분하지만, 비균등 스케일에서 법선이 틀어지는 것을
  나중에 디버깅하는 것보다 지금 제대로 넣는 편이 낫다.
- **`gLightDirection`은 빛이 나아가는 방향** — 셰이더에서 부호를 뒤집어 쓴다.
  방향의 의미를 헷갈리면 물체가 반대편만 밝아지는데 원인을 찾기 어렵다. 주석으로 명시했다.
- **루트 시그니처는 매 프레임 다시 설정** — 커맨드 리스트를 Reset하면 지워진다.
  PSO는 `Reset`의 두 번째 인자로 함께 넘긴다.
- **`Renderer::CreateMesh`가 제출·대기·임시버퍼 해제를 한 번에 처리** — Mesh의 3단계 사용 순서
  (기록 → 실행 → Flush → `DisposeUploaders`)를 호출부가 매번 신경 쓰지 않아도 되게 감쌌다.
- **`FXMMATRIX` 대신 `const XMMATRIX&`** — `FXMMATRIX`는 `__vectorcall` 사용 여부에 따라
  값 전달과 참조 전달이 달라진다. `XM_CALLCONV`를 쓰지 않는 일반 멤버 함수에서는
  명시적 const 참조가 규약 문제를 완전히 피할 수 있어 더 안전하다.
- **리사이즈 시 카메라 종횡비도 갱신** — 이걸 빠뜨리면 창을 가로로 늘렸을 때 큐브가 찌그러진다.
- **회전은 프레임 수가 아니라 경과 시간에 비례** — fps가 달라져도 회전 속도가 같다.
  `XMScalarModAngle`로 각도가 무한정 커지는 것도 막았다.
- **셰이더는 `None` 항목으로 등록** — 런타임 컴파일이므로 MSBuild가 빌드하지 않아야 한다.
  솔루션 탐색기에는 보이되 빌드 대상은 아니게 된다.

### 작성 중 자체 점검 (이 환경에서는 컴파일 불가)
현재 세션은 Linux라 MSVC/Windows SDK가 없어 실제 빌드로 검증할 수 없다. 대신 다음을 스크립트로 대조했다.
- `ConstantBuffers.h` ↔ `Common.hlsli`: 필드 크기·순서·총 바이트(144B / 128B) 일치, 16B 정렬 확인
- `Vertex.h` 입력 레이아웃 ↔ `VertexIn` 시맨틱/포맷 4개 전부 일치
- 헤더에 선언된 멤버 함수 중 정의 누락 0건 (클래스 15개)
- `.vcxproj` / `.filters`의 파일 목록이 디스크와 1:1 일치 (41개)

### 이 단계의 확인 방법
실행하면 짙은 남색 배경에 여섯 면이 서로 다른 색인 큐브가 천천히 회전한다.
Space로 회전 정지/재개, V로 수직 동기화 토글, ESC로 종료. 창 크기를 바꿔도 찌그러지지 않는다.
