#include "stdafx.h"
#include "Texture.h"

#include "../GPUMemory/Resource.h"
#include "../CommandInterface.h"
#include "../GPUMemory/ShaderResourceView.h"
#include "../DescriptorHeap.h"

Texture::Texture(const std::wstring& filePath)
{
	m_FilePath = filePath;
}

Texture::~Texture()
{
	if (m_pImageData != nullptr)
	{
		delete m_pImageData;
	}

	if (m_pDefaultResource != nullptr)
	{
		delete m_pDefaultResource;
	}

	if (m_pUploadResource != nullptr)
	{
		delete m_pUploadResource;
	}

	if (m_pSRV != nullptr)
	{
		delete m_pSRV;
	}
}

const std::wstring& Texture::GetPath() const
{
	return m_FilePath;
}

TEXTURE_TYPE Texture::GetType() const
{
	return m_Type;
}

const unsigned int Texture::GetDescriptorHeapIndex() const
{
	return m_pSRV->GetDescriptorHeapIndex();
}

BYTE* Texture::GetData() const
{
	return m_pImageData;
}

unsigned int Texture::GetWidth() const
{
	return m_ResourceDescription.Width;
}

unsigned int Texture::GetHeight() const
{
	return m_ResourceDescription.Height;
}
