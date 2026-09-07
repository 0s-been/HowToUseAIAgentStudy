// Basic_VS.hlsl
// 정점을 월드 -> 클립 공간으로 옮기고, 조명 계산에 필요한 값을 픽셀 셰이더로 넘긴다.

#include "Common.hlsli"

VertexOut main(VertexIn vin)
{
	VertexOut vout;

	// 행벡터 규약이므로 정점이 왼쪽, 행렬이 오른쪽에 온다.
	const float4 posW = mul(float4(vin.posL, 1.0f), gWorld);
	vout.posW = posW.xyz;
	vout.posH = mul(posW, gViewProj);

	// 법선에는 이동 성분이 필요 없으므로 3x3만 쓴다.
	// 비균등 스케일에서도 법선이 표면에 수직으로 남도록 역전치 행렬을 쓴다.
	vout.normalW = mul(vin.normalL, (float3x3)gWorldInvTranspose);

	vout.color = vin.color * gBaseColor;
	vout.uv = vin.uv;

	return vout;
}
