#include "GeometryGenerator.h"

#include "../Core/Logger.h"

#include <cmath>

using namespace DirectX;

namespace
{
	// 면을 구분해서 볼 수 있도록 여섯 면에 서로 다른 색을 준다.
	constexpr XMFLOAT4 kFaceColors[6] =
	{
		{ 0.90f, 0.30f, 0.30f, 1.0f },	// 앞
		{ 0.30f, 0.75f, 0.45f, 1.0f },	// 뒤
		{ 0.35f, 0.55f, 0.95f, 1.0f },	// 위
		{ 0.95f, 0.80f, 0.30f, 1.0f },	// 아래
		{ 0.75f, 0.45f, 0.90f, 1.0f },	// 왼쪽
		{ 0.35f, 0.80f, 0.85f, 1.0f },	// 오른쪽
	};
}

MeshData GeometryGenerator::CreateBox(float width, float height, float depth)
{
	MeshData mesh;

	const float w = 0.5f * width;
	const float h = 0.5f * height;
	const float d = 0.5f * depth;

	mesh.vertices =
	{
		// 앞면 (법선 -Z)
		{ { -w, -h, -d }, {  0.0f,  0.0f, -1.0f }, kFaceColors[0], { 0.0f, 1.0f } },
		{ { -w, +h, -d }, {  0.0f,  0.0f, -1.0f }, kFaceColors[0], { 0.0f, 0.0f } },
		{ { +w, +h, -d }, {  0.0f,  0.0f, -1.0f }, kFaceColors[0], { 1.0f, 0.0f } },
		{ { +w, -h, -d }, {  0.0f,  0.0f, -1.0f }, kFaceColors[0], { 1.0f, 1.0f } },

		// 뒷면 (법선 +Z)
		{ { -w, -h, +d }, {  0.0f,  0.0f,  1.0f }, kFaceColors[1], { 1.0f, 1.0f } },
		{ { +w, -h, +d }, {  0.0f,  0.0f,  1.0f }, kFaceColors[1], { 0.0f, 1.0f } },
		{ { +w, +h, +d }, {  0.0f,  0.0f,  1.0f }, kFaceColors[1], { 0.0f, 0.0f } },
		{ { -w, +h, +d }, {  0.0f,  0.0f,  1.0f }, kFaceColors[1], { 1.0f, 0.0f } },

		// 윗면 (법선 +Y)
		{ { -w, +h, -d }, {  0.0f,  1.0f,  0.0f }, kFaceColors[2], { 0.0f, 1.0f } },
		{ { -w, +h, +d }, {  0.0f,  1.0f,  0.0f }, kFaceColors[2], { 0.0f, 0.0f } },
		{ { +w, +h, +d }, {  0.0f,  1.0f,  0.0f }, kFaceColors[2], { 1.0f, 0.0f } },
		{ { +w, +h, -d }, {  0.0f,  1.0f,  0.0f }, kFaceColors[2], { 1.0f, 1.0f } },

		// 아랫면 (법선 -Y)
		{ { -w, -h, -d }, {  0.0f, -1.0f,  0.0f }, kFaceColors[3], { 1.0f, 1.0f } },
		{ { +w, -h, -d }, {  0.0f, -1.0f,  0.0f }, kFaceColors[3], { 0.0f, 1.0f } },
		{ { +w, -h, +d }, {  0.0f, -1.0f,  0.0f }, kFaceColors[3], { 0.0f, 0.0f } },
		{ { -w, -h, +d }, {  0.0f, -1.0f,  0.0f }, kFaceColors[3], { 1.0f, 0.0f } },

		// 왼쪽면 (법선 -X)
		{ { -w, -h, +d }, { -1.0f,  0.0f,  0.0f }, kFaceColors[4], { 0.0f, 1.0f } },
		{ { -w, +h, +d }, { -1.0f,  0.0f,  0.0f }, kFaceColors[4], { 0.0f, 0.0f } },
		{ { -w, +h, -d }, { -1.0f,  0.0f,  0.0f }, kFaceColors[4], { 1.0f, 0.0f } },
		{ { -w, -h, -d }, { -1.0f,  0.0f,  0.0f }, kFaceColors[4], { 1.0f, 1.0f } },

		// 오른쪽면 (법선 +X)
		{ { +w, -h, -d }, {  1.0f,  0.0f,  0.0f }, kFaceColors[5], { 0.0f, 1.0f } },
		{ { +w, +h, -d }, {  1.0f,  0.0f,  0.0f }, kFaceColors[5], { 0.0f, 0.0f } },
		{ { +w, +h, +d }, {  1.0f,  0.0f,  0.0f }, kFaceColors[5], { 1.0f, 0.0f } },
		{ { +w, -h, +d }, {  1.0f,  0.0f,  0.0f }, kFaceColors[5], { 1.0f, 1.0f } },
	};

	// 면마다 정점 4개가 연속으로 놓여 있으므로 삼각형 2개를 같은 패턴으로 만든다.
	// 왼손 좌표계에서 앞면이 되도록 시계 방향으로 감는다.
	mesh.indices.reserve(36);
	for (uint16_t face = 0; face < 6; ++face)
	{
		const uint16_t base = static_cast<uint16_t>(face * 4);
		mesh.indices.push_back(static_cast<uint16_t>(base + 0));
		mesh.indices.push_back(static_cast<uint16_t>(base + 1));
		mesh.indices.push_back(static_cast<uint16_t>(base + 2));

		mesh.indices.push_back(static_cast<uint16_t>(base + 0));
		mesh.indices.push_back(static_cast<uint16_t>(base + 2));
		mesh.indices.push_back(static_cast<uint16_t>(base + 3));
	}

	return mesh;
}

MeshData GeometryGenerator::CreateGrid(float width, float depth, uint32_t rowCount, uint32_t columnCount)
{
	MeshData mesh;

	rowCount = std::max(1u, rowCount);
	columnCount = std::max(1u, columnCount);

	// 공유 정점 격자라 정점 수는 (row+1) x (col+1)이다. 인덱스가 uint16_t라
	// 65,536개를 넘을 수 없다. 넘으면 실패시키는 대신 가로세로 비율을 유지한 채
	// 줄이고 경고를 남긴다.
	uint64_t vertexCount = static_cast<uint64_t>(rowCount + 1) * (columnCount + 1);
	if (vertexCount > kMaxGridVertices)
	{
		const float scale = std::sqrt(static_cast<float>(kMaxGridVertices) / static_cast<float>(vertexCount));
		const uint32_t newRows = std::max(1u, static_cast<uint32_t>(static_cast<float>(rowCount) * scale));
		const uint32_t newCols = std::max(1u, static_cast<uint32_t>(static_cast<float>(columnCount) * scale));

		LOG_WARN(L"격자 정점 수가 uint16_t 인덱스 한계를 넘는다. 칸 %u x %u -> %u x %u 로 줄인다.",
			rowCount, columnCount, newRows, newCols);

		rowCount = newRows;
		columnCount = newCols;
		vertexCount = static_cast<uint64_t>(rowCount + 1) * (columnCount + 1);
	}

	const float halfWidth = 0.5f * width;
	const float halfDepth = 0.5f * depth;
	const float cellWidth = width / static_cast<float>(columnCount);
	const float cellDepth = depth / static_cast<float>(rowCount);

	mesh.vertices.reserve(static_cast<size_t>(vertexCount));

	const XMFLOAT3 normal = { 0.0f, 1.0f, 0.0f };
	// 색은 여기서 정하지 않는다(흰색 = 곱해도 그대로). 실제 색/체커 무늬는
	// Renderer::DrawMesh에 넘기는 값으로 픽셀 셰이더가 결정한다.
	const XMFLOAT4 white = { 1.0f, 1.0f, 1.0f, 1.0f };

	// row는 Z(깊이) 방향, column은 X(너비) 방향으로 격자점을 순서대로 둔다.
	for (uint32_t row = 0; row <= rowCount; ++row)
	{
		const float z = -halfDepth + static_cast<float>(row) * cellDepth;
		const float v = static_cast<float>(row) / static_cast<float>(rowCount);

		for (uint32_t column = 0; column <= columnCount; ++column)
		{
			const float x = -halfWidth + static_cast<float>(column) * cellWidth;
			const float u = static_cast<float>(column) / static_cast<float>(columnCount);

			mesh.vertices.push_back({ { x, 0.0f, z }, normal, white, { u, v } });
		}
	}

	mesh.indices.reserve(static_cast<size_t>(rowCount) * columnCount * 6);

	const uint32_t rowStride = columnCount + 1;
	for (uint32_t row = 0; row < rowCount; ++row)
	{
		for (uint32_t column = 0; column < columnCount; ++column)
		{
			// 이 칸 네 모서리의 정점 인덱스.
			//   i0 --- i1
			//   |       |
			//   i2 --- i3
			const uint16_t i0 = static_cast<uint16_t>(row * rowStride + column);
			const uint16_t i1 = static_cast<uint16_t>(i0 + 1);
			const uint16_t i2 = static_cast<uint16_t>(i0 + rowStride);
			const uint16_t i3 = static_cast<uint16_t>(i2 + 1);

			// 큐브 윗면과 같은 감기 순서: (x-,z-) -> (x-,z+) -> (x+,z+) -> (x+,z-)
			mesh.indices.push_back(i0);
			mesh.indices.push_back(i2);
			mesh.indices.push_back(i3);

			mesh.indices.push_back(i0);
			mesh.indices.push_back(i3);
			mesh.indices.push_back(i1);
		}
	}

	return mesh;
}

MeshData GeometryGenerator::CreatePlane(float width, float depth)
{
	// 칸이 하나뿐인 격자가 곧 평면이다. 같은 계산을 두 번 쓰지 않도록 위임한다.
	return CreateGrid(width, depth, 1, 1);
}
