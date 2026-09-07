// Basic_PS.hlsl
// 방향광 하나에 대한 Blinn-Phong 조명.
// 텍스처는 아직 쓰지 않고 정점 색만 사용한다.

#include "Common.hlsli"

float4 main(VertexOut pin) : SV_Target
{
	// 보간을 거치면서 길이가 1이 아니게 되므로 다시 정규화한다.
	const float3 normalW = normalize(pin.normalW);

	// gLightDirection은 빛이 '나아가는' 방향이다.
	// 조명 계산에는 표면에서 광원을 향하는 방향이 필요하므로 부호를 뒤집는다.
	const float3 toLight = normalize(-gLightDirection);

	const float ndotl = max(dot(normalW, toLight), 0.0f);

	const float3 ambient = pin.color.rgb * gAmbientColor.rgb;
	const float3 diffuse = pin.color.rgb * gLightColor.rgb * ndotl;

	// Blinn-Phong 스펙큘러: 시선과 광원의 중간 벡터를 쓴다.
	const float3 toEye = normalize(gEyePosW - pin.posW);
	const float3 halfVec = normalize(toEye + toLight);
	// 뒷면이 반짝이지 않도록 ndotl을 곱해 둔다.
	const float specular = pow(max(dot(normalW, halfVec), 0.0f), 64.0f) * ndotl;

	const float3 color = ambient + diffuse + gLightColor.rgb * specular * 0.35f;

	return float4(color, pin.color.a);
}
