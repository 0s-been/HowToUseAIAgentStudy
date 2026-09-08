// Basic_PS.hlsl
// 방향광 하나에 대한 Blinn-Phong 조명.
// 텍스처는 아직 쓰지 않고 정점 색(또는 절차적 체커 무늬)만 사용한다.

#include "Common.hlsli"

// 체커 무늬를 화면 공간 도함수 기반으로 미리 흐려서 계산한다(box filter 적분).
// posXZ가 화면에서 1픽셀 움직일 때 얼마나 변하는지(ddx/ddy)를 알면,
// 그 폭 안에서 무늬가 평균적으로 얼마나 밝은지를 적분으로 정확히 구할 수 있다.
// 무늬가 화면상에서 아주 작아지는 먼 거리에서는 이 값이 자연스럽게 0.5(회색)로 수렴해서,
// 매 프레임 카메라가 조금만 움직여도 경계가 들쭉날쭉 나타났다 사라지는 현상이 없어진다.
// (참고: MSAA는 "한 삼각형과 다른 삼각형의 경계"만 정해진 샘플 수로 안티앨리어싱한다.
//  체커 칸 하나가 화면 1픽셀보다 작아지면 그 몇 안 되는 샘플만으로는 무늬를 다 담지 못해
//  MSAA를 켜도 이 문제가 완전히 없어지지 않는다. 이 함수는 그 근본 원인을 직접 해결한다.)
float FilteredChecker(float2 posXZ, float cellSize)
{
	const float2 p = posXZ / cellSize;
	const float2 w = max(abs(ddx(p)), abs(ddy(p))) + 1e-4f;

	// 폭 w인 박스 필터를 삼각파(체커의 1차원 성분)에 적분한 결과의 닫힌 해.
	const float2 i = 2.0f * (abs(frac((p - 0.5f * w) * 0.5f) - 0.5f)
	                        - abs(frac((p + 0.5f * w) * 0.5f) - 0.5f)) / w;

	// 두 축의 결과를 XOR 형태로 합치면 체커 패턴이 된다.
	return 0.5f - 0.5f * i.x * i.y;
}

float4 main(VertexOut pin) : SV_Target
{
	// 체커가 꺼져 있으면(gCheckerCellSize <= 0) 정점 색을 그대로 쓴다.
	float3 baseColor = pin.color.rgb;
	if (gCheckerCellSize > 0.0f)
	{
		const float checker = FilteredChecker(pin.posW.xz, gCheckerCellSize);
		// pin.color는 정점 셰이더에서 이미 gBaseColor가 곱해진 값이라 첫 번째 체커 색으로 쓸 수 있다.
		baseColor = lerp(pin.color.rgb, gCheckerColorB.rgb, checker);
	}

	// 보간을 거치면서 길이가 1이 아니게 되므로 다시 정규화한다.
	const float3 normalW = normalize(pin.normalW);

	// gLightDirection은 빛이 '나아가는' 방향이다.
	// 조명 계산에는 표면에서 광원을 향하는 방향이 필요하므로 부호를 뒤집는다.
	const float3 toLight = normalize(-gLightDirection);

	const float ndotl = max(dot(normalW, toLight), 0.0f);

	const float3 ambient = baseColor * gAmbientColor.rgb;
	const float3 diffuse = baseColor * gLightColor.rgb * ndotl;

	// Blinn-Phong 스펙큘러: 시선과 광원의 중간 벡터를 쓴다.
	const float3 toEye = normalize(gEyePosW - pin.posW);
	const float3 halfVec = normalize(toEye + toLight);
	// 뒷면이 반짝이지 않도록 ndotl을 곱해 둔다.
	const float specular = pow(max(dot(normalW, halfVec), 0.0f), 64.0f) * ndotl;

	const float3 color = ambient + diffuse + gLightColor.rgb * specular * 0.35f;

	return float4(color, pin.color.a);
}
