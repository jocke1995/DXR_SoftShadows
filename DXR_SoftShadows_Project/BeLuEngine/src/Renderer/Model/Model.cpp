#include "stdafx.h"
#include "GPU_Structs.h"
#include "Model.h"

#include "Mesh.h"
#include "Material.h"
#include "../Texture/Texture2D.h"
#include "../GPUMemory/GPUMemory.h"

#include "Core.h"
#include <filesystem>

Model::Model(
	const std::wstring* path,
	std::vector<Mesh*>* meshes,
	std::vector<Material*>* materials,
	ID3D12Device5* device,
	DescriptorHeap* dHeap)
{
	m_Path = *path;
	m_Size = (*meshes).size();

	m_Meshes = (*meshes);
	m_Materials = (*materials);

	// Fill slotinfo with empty slotinfos
	m_SlotInfos.resize(m_Size);
	updateSlotInfo();

	D3D12_SHADER_RESOURCE_VIEW_DESC rawBufferDesc = {};
	rawBufferDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	rawBufferDesc.Buffer.FirstElement = 0;
	rawBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	rawBufferDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	rawBufferDesc.Buffer.NumElements = m_Size;
	rawBufferDesc.Buffer.StructureByteStride = 0;
	rawBufferDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

	std::string fileName = std::filesystem::path(to_string(m_Path)).filename().string();
	m_SlotInfoByteAdressBuffer = new ShaderResource(device, sizeof(SlotInfo) * m_Size, to_wstring(fileName) + L"_RAWBUFFER_SLOTINFO", &rawBufferDesc, dHeap);
}

Model::~Model()
{
	delete m_SlotInfoByteAdressBuffer;
}

bool Model::operator==(const Model& other)
{
	return m_Path == other.m_Path;
}

bool Model::operator!=(const Model& other)
{
	return !(operator==(other));
}

const std::wstring& Model::GetPath() const
{
	return m_Path;
}

unsigned int Model::GetSize() const
{
	return m_Size;
}

Mesh* Model::GetMeshAt(unsigned int index) const
{
	return m_Meshes[index];
}

void Model::SetMeshAt(unsigned int index, Mesh* mesh)
{
	m_Meshes[index] = mesh;
	updateSlotInfo();
}

Material* Model::GetMaterialAt(unsigned int index) const
{
	return m_Materials[index];
}

void Model::SetMaterialAt(unsigned int index, Material* material)
{
	m_Materials[index] = material;
	updateSlotInfo();
}

const SlotInfo* Model::GetSlotInfoAt(unsigned int index) const
{
	return &m_SlotInfos[index];
}

void Model::SetBottomLevelResult(ID3D12Resource1* blResult)
{
	m_pBottomLevelResult = blResult;
}

ID3D12Resource1* Model::GetBottomLevelResultP() const
{
	return m_pBottomLevelResult;
}

ShaderResource* Model::GetByteAdressInfoDXR() const
{
	return m_SlotInfoByteAdressBuffer;
}

void Model::updateSlotInfo()
{
#ifdef DEBUG
	if (m_Meshes[0]->m_pVertexBufferSRV == nullptr || m_Materials[0]->GetTexture(TEXTURE2D_TYPE::ALBEDO)->m_pSRV == nullptr)
	{
		BL_LOG_CRITICAL("Model.cpp::updateSlotInfo got unInit:ed variables\n");
	}
#endif // DEBUG

	for (unsigned int i = 0; i < m_Size; i++)
	{
		m_SlotInfos[i] =
		{
			m_Meshes[i]->m_pVertexBufferSRV->GetDescriptorHeapIndex(),
			m_Meshes[i]->m_pIndexBufferSRV->GetDescriptorHeapIndex(),
			m_Materials[i]->GetTexture(TEXTURE2D_TYPE::ALBEDO)->GetDescriptorHeapIndex(),
			m_Materials[i]->GetTexture(TEXTURE2D_TYPE::ROUGHNESS)->GetDescriptorHeapIndex(),
			m_Materials[i]->GetTexture(TEXTURE2D_TYPE::METALLIC)->GetDescriptorHeapIndex(),
			m_Materials[i]->GetTexture(TEXTURE2D_TYPE::NORMAL)->GetDescriptorHeapIndex(),
			m_Materials[i]->GetTexture(TEXTURE2D_TYPE::EMISSIVE)->GetDescriptorHeapIndex(),
			m_Materials[i]->GetTexture(TEXTURE2D_TYPE::OPACITY)->GetDescriptorHeapIndex()
		};
	}
}

