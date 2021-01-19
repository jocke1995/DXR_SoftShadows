#ifndef TEXTURE_H
#define TEXTURE_H

#include "Core.h"
#include "../../Headers/d3dx12.h"

class Resource;
class CommandInterface;
class ShaderResourceView;
class DescriptorHeap;

class Texture
{
public:
	Texture(const std::wstring& filePath);
	virtual ~Texture();

	virtual bool Init(ID3D12Device5* device, DescriptorHeap* descriptorHeap) = 0;

	const std::wstring& GetPath() const;
	TEXTURE_TYPE GetType() const;
	const unsigned int GetDescriptorHeapIndex() const;
	BYTE* GetData() const;
	unsigned int GetWidth() const;
	unsigned int GetHeight() const;

protected:
	// CopyOnDemandTask & Renderer uses the private members of the texture class to upload data to the gpu
	friend class CopyOnDemandTask;
	friend class Renderer;
	friend class Model;

	TEXTURE_TYPE m_Type = TEXTURE_TYPE::UNKNOWN;

	unsigned char* m_pImageData = nullptr;
	int m_ImageBytesPerRow = 0;
	std::wstring m_FilePath = L"";
	ShaderResourceView* m_pSRV = nullptr;
	std::vector<D3D12_SUBRESOURCE_DATA> m_SubresourceData;
	D3D12_RESOURCE_DESC m_ResourceDescription = {};
	Resource* m_pDefaultResource = nullptr;
	Resource* m_pUploadResource = nullptr;
};

#endif
