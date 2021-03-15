#include "stdafx.h"
#include "Mesh.h"

#include "../GPUMemory/GPUMemory.h"
#include "../DescriptorHeap.h"
#include "../Texture/Texture.h"

Mesh::Mesh(std::vector<Vertex>* vertices, std::vector<unsigned int>* indices, const std::wstring& path)
{
	m_Id = s_IdCounter++;

	m_Path = path + L"_Mesh_ID_" + std::to_wstring(m_Id);
	m_Vertices = *vertices;
	m_Indices = *indices;
}

Mesh::~Mesh()
{
	if (m_pUploadResourceVertices != nullptr)
	{
		delete m_pUploadResourceVertices;
	}
	

	if (m_pDefaultResourceVertices != nullptr)
	{
		delete m_pDefaultResourceVertices;
	}
	

	// Set indices
	if (m_pUploadResourceIndices != nullptr)
	{
		delete m_pUploadResourceIndices;
	}
	
	if (m_pDefaultResourceIndices != nullptr)
	{
		delete m_pDefaultResourceIndices;
	}

	if (m_pVertexBufferSRV != nullptr)
	{
		delete m_pVertexBufferSRV;
	}

	if (m_pIndexBufferSRV != nullptr)
	{
		delete m_pIndexBufferSRV;
	}

	if (m_pIndexBufferView != nullptr)
	{
		delete m_pIndexBufferView;
	}
}

bool Mesh::operator==(const Mesh& other)
{
	return m_Id == other.m_Id;
}

bool Mesh::operator!=(const Mesh& other)
{
	return !(operator==(other));
}

void Mesh::Init(ID3D12Device5* m_pDevice5, DescriptorHeap* CBV_UAV_SRV_heap)
{
	std::string modelPathName = to_string(m_Path);
	modelPathName = modelPathName.substr(modelPathName.find_last_of("/\\") + 1);

	// create vertices resource
	m_pUploadResourceVertices = new Resource(m_pDevice5, GetSizeOfVertices(), RESOURCE_TYPE::UPLOAD, L"VERTEX_UPLOAD_RESOURCE_" + to_wstring(modelPathName));
	m_pDefaultResourceVertices = new Resource(m_pDevice5, GetSizeOfVertices(), RESOURCE_TYPE::DEFAULT, L"VERTEX_DEFAULT_RESOURCE_" + to_wstring(modelPathName));

	// Create VB SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC dsrv = {};
	dsrv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	dsrv.Buffer.FirstElement = 0;
	dsrv.Format = DXGI_FORMAT_UNKNOWN;
	dsrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	dsrv.Buffer.NumElements = GetNumVertices();
	dsrv.Buffer.StructureByteStride = sizeof(Vertex);

	// Set view to mesh
	m_pVertexBufferSRV = new ShaderResourceView(
		m_pDevice5,
		CBV_UAV_SRV_heap,
		&dsrv,
		m_pDefaultResourceVertices);

	// Set indices resource
	m_pUploadResourceIndices = new Resource(m_pDevice5, GetSizeOfIndices(), RESOURCE_TYPE::UPLOAD, L"INDEX_UPLOAD_RESOURCE_" + to_wstring(modelPathName));
	m_pDefaultResourceIndices = new Resource(m_pDevice5, GetSizeOfIndices(), RESOURCE_TYPE::DEFAULT, L"INDEX_DEFAULT_RESOURCE_" + to_wstring(modelPathName));

	// Set indexBufferView
	m_pIndexBufferView = new D3D12_INDEX_BUFFER_VIEW();
	m_pIndexBufferView->BufferLocation = m_pDefaultResourceIndices->GetGPUVirtualAdress();
	m_pIndexBufferView->Format = DXGI_FORMAT_R32_UINT;
	m_pIndexBufferView->SizeInBytes = GetSizeOfIndices();

	// Create IB SRV
	dsrv = {};
	dsrv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	dsrv.Buffer.FirstElement = 0;
	dsrv.Format = DXGI_FORMAT_UNKNOWN;
	dsrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	dsrv.Buffer.NumElements = GetNumIndices();
	dsrv.Buffer.StructureByteStride = sizeof(unsigned int);

	m_pIndexBufferSRV = new ShaderResourceView(
		m_pDevice5,
		CBV_UAV_SRV_heap,
		&dsrv,
		m_pDefaultResourceIndices);
}


Resource* Mesh::GetDefaultResourceVertices() const
{
	return m_pDefaultResourceVertices;
}

const std::vector<Vertex>* Mesh::GetVertices() const
{
	return &m_Vertices;
}

const unsigned int Mesh::GetSizeOfVertices() const
{
	return m_Vertices.size() * sizeof(Vertex);
}

const unsigned int Mesh::GetNumVertices() const
{
	return m_Vertices.size();
}

Resource* Mesh::GetDefaultResourceIndices() const
{
	return m_pDefaultResourceIndices;
}

const std::vector<unsigned int>* Mesh::GetIndices() const
{
	return &m_Indices;
}

const unsigned int Mesh::GetSizeOfIndices() const
{
	return m_Indices.size() * sizeof(unsigned int);
}

const unsigned int Mesh::GetNumIndices() const
{
	return m_Indices.size();
}

const D3D12_INDEX_BUFFER_VIEW* Mesh::GetIndexBufferView() const
{
	return m_pIndexBufferView;
}

const std::wstring& Mesh::GetPath() const
{
	return m_Path;
}

ShaderResourceView* const Mesh::GetVBSRV() const
{
	return m_pVertexBufferSRV;
}

ShaderResourceView* const Mesh::GetIBSRV() const
{
	return m_pIndexBufferSRV;
}
