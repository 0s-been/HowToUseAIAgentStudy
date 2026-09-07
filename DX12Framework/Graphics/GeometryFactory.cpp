#include "GeometryFactory.h"

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

MeshData GeometryFactory::CreateBox(float width, float height, float depth)
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
