#ifndef MODEL_H
#define MODEL_H

class Mesh;
class Material;
class Resource;
class ShaderResource;
class ShaderResourceView;
class DescriptorHeap;
struct SlotInfo;

// DX12 Forward Declarations
struct ID3D12Device5;
struct D3D12_INDEX_BUFFER_VIEW;

class Model
{
public:
    Model(const std::wstring* path,
        std::vector<Mesh*>* meshes,
        std::vector<Material*>* materials,
        ID3D12Device5* device,
        DescriptorHeap* dHeap);
    virtual ~Model();

    bool operator == (const Model& other);
    bool operator != (const Model& other);

    const std::wstring& GetPath() const;
    unsigned int GetSize() const;

    // Mesh
    Mesh* GetMeshAt(unsigned int index) const;
    void SetMeshAt(unsigned int index, Mesh* mesh);

    // Material
    Material* GetMaterialAt(unsigned int index) const;
    void SetMaterialAt(unsigned int index, Material* material);

    // SlotInfo
    const SlotInfo* GetSlotInfoAt(unsigned int index) const;

    // DXR
    void SetBottomLevelResult(ID3D12Resource1* blResult);
    ID3D12Resource1* GetBottomLevelResultP() const;
    ShaderResource* GetByteAdressInfoDXR() const;

protected:
    friend class Renderer;
    friend class AssetLoader;

    void updateSlotInfo();

    std::wstring m_Path;
    unsigned int m_Size = 0;

    std::vector<Mesh*> m_Meshes;
    std::vector<Material*> m_Materials;
    std::vector<SlotInfo> m_SlotInfos;

    // DXR
    ID3D12Resource1* m_pBottomLevelResult = nullptr;
    ShaderResource* m_SlotInfoByteAdressBuffer;

    unsigned int m_DXRStartDHIndex = 0;
};

#endif
