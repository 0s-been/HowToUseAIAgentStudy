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

MeshData GeometryGenerator::CreateGrid(float width, float depth,
	uint32_t rowCount, uint32_t columnCount,
	const XMFLOAT4& colorA, const XMFLOAT4& colorB)
{
	MeshData mesh;

	rowCount = std::max(1u, rowCount);
	columnCount = std::max(1u, columnCount);

	// 칸이 너무 많으면 uint16_t 인덱스 범위를 넘는다.
	// 실패시키는 대신 가로세로 비율을 유지한 채 줄이고 경고를 남긴다.
	const uint64_t cellCount = static_cast<uint64_t>(rowCount) * columnCount;
	if (cellCount > kMaxGridCells)
	{
		const float scale = std::sqrt(static_cast<float>(kMaxGridCells) / static_cast<float>(cellCount));
		const uint32_t newRows = std::max(1u, static_cast<uint32_t>(static_cast<float>(rowCount) * scale));
		const uint32_t newCols = std::max(1u, static_cast<uint32_t>(static_cast<float>(columnCount) * scale));

		LOG_WARN(L"격자 칸 수가 uint16_t 인덱스 한계를 넘는다. %u x %u -> %u x %u 로 줄인다.",
			rowCount, columnCount, newRows, newCols);

		rowCount = newRows;
		columnCount = newCols;
	}

	const float halfWidth = 0.5f * width;
	const float halfDepth = 0.5f * depth;
	const float cellWidth = width / static_cast<float>(columnCount);
	const float cellDepth = depth / static_cast<float>(rowCount);

	mesh.vertices.reserve(static_cast<size_t>(rowCount) * columnCount * 4);
	mesh.indices.reserve(static_cast<size_t>(rowCount) * columnCount * 6);

	const XMFLOAT3 normal = { 0.0f, 1.0f, 0.0f };

	for (uint32_t row = 0; row < rowCount; ++row)
	{
		for (uint32_t column = 0; column < columnCount; ++column)
		{
			const float x0 = -halfWidth + static_cast<float>(column) * cellWidth;
			const float x1 = x0 + cellWidth;
			const float z0 = -halfDepth + static_cast<float>(row) * cellDepth;
			const float z1 = z0 + cellDepth;

			// 체커 무늬. 인접한 칸끼리 색이 달라진다.
			const XMFLOAT4& color = (((row + column) % 2) == 0) ? colorA : colorB;

			const uint16_t base = static_cast<uint16_t>(mesh.vertices.size());

			// 감는 순서는 큐브의 윗면과 같다.
			// (x-,z-) -> (x-,z+) -> (x+,z+) -> (x+,z-)
			// 왼손 좌표계에서 위쪽(+Y)이 앞면이 되는 순서다.
			mesh.vertices.push_back({ { x0, 0.0f, z0 }, normal, color, { 0.0f, 1.0f } });
			mesh.vertices.push_back({ { x0, 0.0f, z1 }, normal, color, { 0.0f, 0.0f } });
			mesh.vertices.push_back({ { x1, 0.0f, z1 }, normal, color, { 1.0f, 0.0f } });
			mesh.vertices.push_back({ { x1, 0.0f, z0 }, normal, color, { 1.0f, 1.0f } });

			mesh.indices.push_back(static_cast<uint16_t>(base + 0));
			mesh.indices.push_back(static_cast<uint16_t>(base + 1));
			mesh.indices.push_back(static_cast<uint16_t>(base + 2));

			mesh.indices.push_back(static_cast<uint16_t>(base + 0));
			mesh.indices.push_back(static_cast<uint16_t>(base + 2));
			mesh.indices.push_back(static_cast<uint16_t>(base + 3));
		}
	}

	return mesh;
}

MeshData GeometryGenerator::CreatePlane(float width, float depth, const XMFLOAT4& color)
{
	// 칸이 하나뿐이고 두 색이 같은 격자가 곧 평면이다.
	// 같은 계산을 두 번 쓰지 않도록 위임한다.
	return CreateGrid(width, depth, 1, 1, color, color);
}
