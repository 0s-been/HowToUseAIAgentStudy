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
