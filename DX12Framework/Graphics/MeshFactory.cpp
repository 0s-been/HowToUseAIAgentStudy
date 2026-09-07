#include "MeshFactory.h"

#include "../Core/Logger.h"
#include "Renderer.h"

MeshFactory::~MeshFactory()
{
	Shutdown();
}

bool MeshFactory::Initialize(Renderer* renderer)
{
	if (renderer == nullptr)
	{
		LOG_ERROR(L"MeshFactory 초기화 실패: 렌더러가 null이다.");
		return false;
	}

	m_renderer = renderer;
	LOG_INFO(L"메시 팩토리 초기화 완료");
	return true;
}

void MeshFactory::Shutdown()
{
	if (m_meshes.empty())
	{
		m_renderer = nullptr;
		return;
	}

	LOG_INFO(L"메시 팩토리 정리: 메시 %zu개 해제", m_meshes.size());

	for (auto& entry : m_meshes)
	{
		entry.second->Shutdown();
	}
	m_meshes.clear();

	m_renderer = nullptr;
}

Mesh* MeshFactory::Create(const std::wstring& name, const MeshData& data)
{
	if (m_renderer == nullptr)
	{
		LOG_ERROR(L"메시 생성 실패: 팩토리가 초기화되지 않았다. (%s)", name.c_str());
		return nullptr;
	}

	// 같은 이름이 이미 있으면 그대로 재사용한다.
	// 같은 도형을 여러 오브젝트가 공유할 때 GPU 메모리와 업로드 비용을 아낀다.
	if (Mesh* existing = Find(name))
	{
		LOG_TRACE(L"메시 재사용: %s", name.c_str());
		return existing;
	}

	auto mesh = std::make_unique<Mesh>();
	if (!m_renderer->CreateMesh(*mesh, data, name.c_str()))
	{
		LOG_ERROR(L"메시 생성 실패: %s", name.c_str());
		return nullptr;
	}

	// unique_ptr를 옮기기 전에 원시 포인터를 확보해 둔다.
	Mesh* result = mesh.get();
	m_meshes.emplace(name, std::move(mesh));
	return result;
}

Mesh* MeshFactory::CreateBox(const std::wstring& name, float width, float height, float depth)
{
	// 이미 있으면 도형 계산조차 하지 않고 빠져나간다.
	if (Mesh* existing = Find(name))
	{
		return existing;
	}
	return Create(name, GeometryGenerator::CreateBox(width, height, depth));
}

Mesh* MeshFactory::CreateGrid(const std::wstring& name, float width, float depth,
	uint32_t rowCount, uint32_t columnCount,
	const DirectX::XMFLOAT4& colorA, const DirectX::XMFLOAT4& colorB)
{
	if (Mesh* existing = Find(name))
	{
		return existing;
	}
	return Create(name, GeometryGenerator::CreateGrid(width, depth, rowCount, columnCount, colorA, colorB));
}

Mesh* MeshFactory::CreatePlane(const std::wstring& name, float width, float depth,
	const DirectX::XMFLOAT4& color)
{
	if (Mesh* existing = Find(name))
	{
		return existing;
	}
	return Create(name, GeometryGenerator::CreatePlane(width, depth, color));
}

Mesh* MeshFactory::Find(const std::wstring& name) const
{
	const auto it = m_meshes.find(name);
	return (it != m_meshes.end()) ? it->second.get() : nullptr;
}

bool MeshFactory::Destroy(const std::wstring& name)
{
	const auto it = m_meshes.find(name);
	if (it == m_meshes.end())
	{
		return false;
	}

	// GPU가 아직 이 메시를 그리고 있을 수 있다.
	// 호출부에서 Renderer::WaitForGpu를 먼저 부르지 않으면 위험하다.
	it->second->Shutdown();
	m_meshes.erase(it);
	return true;
}
