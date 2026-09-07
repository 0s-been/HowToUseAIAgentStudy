// stdafx.h
// DX12Framework 전역 공통 헤더.
// Windows / D3D12 / STL 등 자주 쓰이면서 거의 바뀌지 않는 헤더만 모아둔다.
// (미리 컴파일된 헤더(PCH)로는 설정하지 않았다. 빌드 설정을 단순하게 유지하기 위함.)

#pragma once

// Windows 헤더의 불필요한 부분과 min/max 매크로를 제외한다.
// (min/max 매크로는 std::min/std::max, DirectXMath와 충돌한다.)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <wrl/client.h>

// Direct3D 12 / DXGI
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

// COM 객체 수명 관리는 전부 ComPtr로 통일한다.
using Microsoft::WRL::ComPtr;

// 백버퍼 개수. 3장(트리플 버퍼링)이면 CPU가 GPU를 기다리며 노는 시간을 줄일 수 있다.
constexpr UINT kFrameBufferCount = 3;
