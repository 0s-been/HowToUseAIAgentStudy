// GeometryGenerator.h
// 코드로 기본 도형의 정점/인덱스 데이터를 만든다.
//
// 여기서 다루는 것은 순수한 CPU 데이터뿐이다.
// GPU 버퍼로 올리고 수명을 관리하는 일은 MeshFactory와 Mesh가 맡는다.
// 이렇게 나눠 두면 도형 계산을 GPU 없이 단독으로 시험해 볼 수 있고,
// 새 도형을 추가할 때 렌더링 코드를 건드리지 않아도 된다.

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
	// 격자는 칸마다 정점 4개를 쓰므로 칸 수 상한이 이 값이 된다.
	constexpr uint32_t kMaxGridCells = 16384;

	// 원점을 중심으로 하는 직육면체.
	// 면마다 법선이 다르므로 정점을 공유하지 않고 24개를 쓴다.
	// (법선을 공유하면 면의 경계가 부드럽게 뭉개진다.)
	MeshData CreateBox(float width, float height, float depth);

	// XZ 평면 위에 놓이는 격자. 법선은 +Y, 원점이 중심이다.
	// columnCount는 X축 방향 칸 수, rowCount는 Z축 방향 칸 수다.
	//
	// 칸마다 색을 번갈아 주기 위해 정점을 공유하지 않는다(칸당 4개).
	// 정점 색이 칸 단위로 달라야 하는데, 공유하면 경계에서 색이 섞이기 때문이다.
	// 지형처럼 정점을 공유해야 하는 격자가 필요하면 색을 하나로 두고
	// (rowCount+1) x (columnCount+1)개의 정점을 만드는 방식으로 따로 구현하면 된다.
	MeshData CreateGrid(float width, float depth,
		uint32_t rowCount, uint32_t columnCount,
		const DirectX::XMFLOAT4& colorA, const DirectX::XMFLOAT4& colorB);

	// 사각형 하나짜리 평면. 칸이 1개인 격자와 같으므로 CreateGrid에 위임한다.
	MeshData CreatePlane(float width, float depth, const DirectX::XMFLOAT4& color);
}
