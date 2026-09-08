// GeometryGenerator.h
// 코드로 기본 도형의 정점/인덱스 데이터를 만든다.
//
// 여기서 다루는 것은 순수한 CPU 데이터뿐이다.
// GPU 버퍼로 올리고 수명을 관리하는 일은 MeshFactory와 Mesh가 맡는다.
// 이렇게 나눠 두면 도형 계산을 GPU 없이 단독으로 시험해 볼 수 있고,
// 새 도형을 추가할 때 렌더링 코드를 건드리지 않아도 된다.
//
// 색은 여기서 굽지 않는다. 어떤 색으로 칠할지는 셰이딩(그리는 방식)의 문제이지
// 지오메트리(모양)의 문제가 아니다. 체커 무늬 같은 패턴은 Renderer::DrawMesh가
// 받는 파라미터로 픽셀 셰이더에서 계산한다 (Shaders/Basic_PS.hlsl의 FilteredChecker 참고).

#pragma once
#include "../Core/stdafx.h"
#include "Vertex.h"

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
};

namespace GeometryGenerator
{
	// 인덱스가 uint16_t라 정점은 65,536개를 넘을 수 없다.
	constexpr uint32_t kMaxGridVertices = 65536;

	// 원점을 중심으로 하는 직육면체.
	// 면마다 법선이 다르므로 정점을 공유하지 않고 24개를 쓴다.
	// (법선을 공유하면 면의 경계가 부드럽게 뭉개진다.)
	MeshData CreateBox(float width, float height, float depth);

	// XZ 평면 위에 놓이는 격자. 법선은 +Y, 원점이 중심이다.
	// columnCount는 X축 방향 칸 수, rowCount는 Z축 방향 칸 수다.
	//
	// 평면이라 모든 정점의 법선과 색이 같으므로 칸 경계에서도 정점을 그대로 공유한다
	// ((rowCount+1) x (columnCount+1)개). 예전에는 칸마다 색을 다르게 굽기 위해
	// 정점을 공유하지 않았지만, 이제 색은 셰이딩 단계에서 다루므로 그럴 필요가 없다.
	MeshData CreateGrid(float width, float depth, uint32_t rowCount, uint32_t columnCount);

	// 사각형 하나짜리 평면. 칸이 1개인 격자와 같으므로 CreateGrid에 위임한다.
	MeshData CreatePlane(float width, float depth);
}
