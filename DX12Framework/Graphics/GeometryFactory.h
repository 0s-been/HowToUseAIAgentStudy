// GeometryFactory.h
// 코드로 기본 도형의 정점/인덱스 데이터를 만든다.
// GPU 버퍼로 올리는 일은 Mesh가 담당하므로, 여기서는 순수 CPU 데이터만 다룬다.

#pragma once
#include "../Core/stdafx.h"
#include "Vertex.h"

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
};

namespace GeometryFactory
{
	// 원점을 중심으로 하는 직육면체.
	// 면마다 법선이 다르므로 정점을 공유하지 않고 24개를 쓴다.
	// (법선을 공유하면 면의 경계가 부드럽게 뭉개진다.)
	MeshData CreateBox(float width, float height, float depth);
}
