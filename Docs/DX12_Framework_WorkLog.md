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
