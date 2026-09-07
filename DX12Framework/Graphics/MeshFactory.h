// MeshFactory.h
// 메시(GPU 자원)를 만들고, 이름으로 찾고, 수명을 책임지는 팩토리.
//
// [왜 팩토리를 두는가]
// Mesh를 쓰는 쪽에서 직접 만들면 세 가지가 호출부에 흩어진다.
//   1) 도형 데이터를 계산하고
//   2) Renderer로 GPU에 올리고 (제출 -> 대기 -> 임시 버퍼 해제)
//   3) 소멸 시점을 GPU 작업이 끝난 뒤로 맞춰야 한다
// 팩토리가 이 셋을 한곳에서 처리하면 호출부는 "격자 하나 줘"라고만 하면 된다.
// 같은 이름으로 다시 요청하면 이미 만든 것을 돌려주므로 중복 업로드도 막힌다.
//
// [소유권]
// 만들어진 Mesh는 전부 팩토리가 소유한다. 호출부는 관찰용 포인터만 받는다.
// unique_ptr에 담는 이유는, 맵이 재해싱되어도 Mesh의 주소가 변하지 않게 하기 위해서다.
// (Mesh를 값으로 담으면 원소가 이사하면서 이미 나눠준 포인터가 전부 무효가 된다.)

#pragma once
#include "../Core/stdafx.h"
#include "GeometryGenerator.h"
#include "Mesh.h"

#include <unordered_map>

class Renderer;

class MeshFactory
{
public:
	MeshFactory() = default;
	~MeshFactory();

	MeshFactory(const MeshFactory&) = delete;
	MeshFactory& operator=(const MeshFactory&) = delete;

	bool Initialize(Renderer* renderer);

	// 보유한 메시를 모두 해제한다.
	// GPU가 참조 중일 수 있으므로 Renderer::WaitForGpu 뒤에 호출해야 한다.
	void Shutdown();

	// --- 도형별 생성 ---
	// 이미 같은 이름이 있으면 새로 만들지 않고 그것을 돌려준다.
	// 실패하면 nullptr를 반환한다.
	Mesh* CreateBox(const std::wstring& name, float width, float height, float depth);

	Mesh* CreateGrid(const std::wstring& name, float width, float depth,
		uint32_t rowCount, uint32_t columnCount,
		const DirectX::XMFLOAT4& colorA, const DirectX::XMFLOAT4& colorB);

	Mesh* CreatePlane(const std::wstring& name, float width, float depth,
		const DirectX::XMFLOAT4& color);

	// 직접 만든 MeshData를 등록한다.
	// 새 도형을 추가할 때 GeometryGenerator에 함수 하나만 늘리고 이걸 쓰면 된다.
	Mesh* Create(const std::wstring& name, const MeshData& data);

	// 없으면 nullptr.
	Mesh* Find(const std::wstring& name) const;

	// 지운 것이 있으면 true. 호출 이후 해당 메시를 가리키던 포인터는 무효가 된다.
	bool Destroy(const std::wstring& name);

	size_t GetCount() const { return m_meshes.size(); }

private:
	Renderer* m_renderer = nullptr;
	std::unordered_map<std::wstring, std::unique_ptr<Mesh>> m_meshes;
};
