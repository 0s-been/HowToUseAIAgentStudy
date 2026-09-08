// Common.hlsli
// 정점 셰이더와 픽셀 셰이더가 함께 쓰는 상수 버퍼와 구조체 정의.
//
// 여기 있는 cbuffer의 필드 순서와 크기는 Graphics/ConstantBuffers.h의
// C++ 구조체와 정확히 일치해야 한다. 하나만 어긋나도 값이 밀려서 들어간다.
//
// [행렬 규약]
// C++ 쪽에서 XMMatrixTranspose를 거쳐 올리기 때문에, 여기서는 원래 행렬이 그대로 보인다.
// 곱셈 순서는 행벡터 규약을 쓴다: mul(float4(pos, 1.0f), gWorld)

#ifndef COMMON_HLSLI
#define COMMON_HLSLI

// 오브젝트마다 달라지는 값
cbuffer cbObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4   gBaseColor;

	// gCheckerCellSize가 0이면 체커를 쓰지 않는다. (Graphics/ConstantBuffers.h 주석 참고)
	float4   gCheckerColorB;
	float    gCheckerCellSize;
	float3   gCheckerPadding;
};

// 프레임(패스) 전체에서 공유하는 값
cbuffer cbPass : register(b1)
{
	float4x4 gViewProj;

	float3 gEyePosW;
	float  gTotalTime;

	// 빛이 나아가는 방향 (정규화되어 있다)
	float3 gLightDirection;
	float  gPadding;

	float4 gLightColor;
	float4 gAmbientColor;
};

// 시맨틱 이름은 Graphics/Vertex.h의 입력 레이아웃과 반드시 일치해야 한다.
struct VertexIn
{
	float3 posL    : POSITION;
	float3 normalL : NORMAL;
	float4 color   : COLOR;
	float2 uv      : TEXCOORD;
};

struct VertexOut
{
	float4 posH    : SV_POSITION;	// 클립 공간 위치 (필수)
	float3 posW    : POSITION;		// 월드 공간 위치 (스펙큘러 계산에 필요)
	float3 normalW : NORMAL;		// 월드 공간 법선
	float4 color   : COLOR;
	float2 uv      : TEXCOORD;
};

#endif // COMMON_HLSLI
