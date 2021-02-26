
#include "stdafx.h"
#include "AssetLoader.h"

#include "../ECS/Scene.h"
#include "../ECS/Entity.h"

#include "Window.h"
#include "../Renderer/DescriptorHeap.h"
#include "../Renderer/Model/Mesh.h"
#include "../Renderer/Model/Model.h"
#include "../Renderer/Shader.h"
#include "../Renderer/Model/Material.h"
#include "../Renderer/Model/Transform.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/pbrmaterial.h"

#include "../Renderer/Texture/Texture2D.h"
#include "../Renderer/Texture/Texture2DGUI.h"
#include "../Renderer/Texture/TextureCubeMap.h"

#include "EngineMath.h"

AssetLoader::AssetLoader(ID3D12Device5* device, DescriptorHeap* descriptorHeap_CBV_UAV_SRV, const Window* window)
{
	m_pDevice = device;
	m_pDescriptorHeap_CBV_UAV_SRV = descriptorHeap_CBV_UAV_SRV;
	m_pWindow = const_cast<Window*>(window);

	// Load default textures
	loadDefaultMaterial();
}

bool AssetLoader::IsModelLoadedOnGpu(const std::wstring& name) const
{
	return m_LoadedModels.at(name).first;
}

bool AssetLoader::IsModelLoadedOnGpu(const Model* model) const
{
	return m_LoadedModels.at(model->GetPath()).first;
}

bool AssetLoader::IsMaterialLoadedOnGpu(const std::wstring& name) const
{
	return m_LoadedMaterials.at(name).first;
}

bool AssetLoader::IsMaterialLoadedOnGpu(const Material* material) const
{
	return m_LoadedMaterials.at(material->GetPath()).first;
}

bool AssetLoader::IsTextureLoadedOnGpu(const std::wstring& name) const
{
	return m_LoadedTextures.at(name).first;
}

bool AssetLoader::IsTextureLoadedOnGpu(const Texture* texture) const
{
	return m_LoadedTextures.at(texture->GetPath()).first;
}

void AssetLoader::loadDefaultMaterial()
{
	// Load default textures
	std::map<TEXTURE2D_TYPE, Texture*> matTextures;
	matTextures[TEXTURE2D_TYPE::ALBEDO] = LoadTexture2D(m_FilePathDefaultTextures + L"default_albedo.dds");
	matTextures[TEXTURE2D_TYPE::ROUGHNESS] = LoadTexture2D(m_FilePathDefaultTextures + L"default_roughness.dds");
	matTextures[TEXTURE2D_TYPE::METALLIC] = LoadTexture2D(m_FilePathDefaultTextures + L"default_metallic.dds");
	matTextures[TEXTURE2D_TYPE::NORMAL] = LoadTexture2D(m_FilePathDefaultTextures + L"default_normal.dds");
	matTextures[TEXTURE2D_TYPE::EMISSIVE] = LoadTexture2D(m_FilePathDefaultTextures + L"default_emissive.dds");
	matTextures[TEXTURE2D_TYPE::OPACITY] = LoadTexture2D(m_FilePathDefaultTextures + L"default_opacity.dds");

	std::wstring matName = L"DefaultMaterial";
	Material* material = new Material(&matName, &matTextures);
	m_LoadedMaterials[matName].first = false;
	m_LoadedMaterials[matName].second = material;
}

AssetLoader::~AssetLoader()
{
	// For every Mesh
	for (auto mesh : m_LoadedMeshes)
	{
		delete mesh;
	}

	// For every texture
	for (auto pair : m_LoadedTextures)
	{
		delete pair.second.second;
	}

	// For every Material
	for (auto material : m_LoadedMaterials)
	{
		delete material.second.second;
	}

	// For every model
	for (auto pair : m_LoadedModels)
	{
		delete pair.second.second;
	}

	// For every shader
	for (auto shader : m_LoadedShaders)
	{
		delete shader.second;
	}
}

AssetLoader* AssetLoader::Get(ID3D12Device5* device, DescriptorHeap* descriptorHeap_CBV_UAV_SRV, const Window* window)
{
	static AssetLoader instance(device, descriptorHeap_CBV_UAV_SRV, window);

	return &instance;
}

Model* AssetLoader::LoadModel(const std::wstring& path)
{
	// Check if the model already exists
	if (m_LoadedModels.count(path) != 0)
	{
		return m_LoadedModels[path].second;
	}

	// Else load the model
	const std::string filePath(path.begin(), path.end());

	Assimp::Importer importer;
	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, true);
	const aiScene* assimpScene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_GenUVCoords | aiProcess_CalcTangentSpace | aiProcess_ConvertToLeftHanded | aiProcess_OptimizeMeshes | aiProcess_JoinIdenticalVertices | aiProcess_OptimizeGraph);

	if (assimpScene == nullptr)
	{
		BL_LOG_CRITICAL("Failed to load model with path: \'%S\'\n", path.c_str());
		return nullptr;
	}

	std::vector<Mesh*> meshes;
	std::vector<std::map<TEXTURE2D_TYPE, Texture*>> textures;
	std::vector<Material*> materials;

	meshes.reserve(assimpScene->mNumMeshes);
	materials.reserve(assimpScene->mNumMeshes);
	m_LoadedModels[path].first = false;

	processModel(assimpScene, &meshes, &materials, path);
	m_LoadedModels[path].second = new Model(&path, &meshes, &materials, m_pDevice, m_pDescriptorHeap_CBV_UAV_SRV);

	return m_LoadedModels[path].second;
}

Texture* AssetLoader::LoadTexture2D(const std::wstring& path)
{
	// Check if the texture already exists
	if (m_LoadedTextures.count(path) != 0)
	{
		return m_LoadedTextures[path].second;
	}

	// Check if the texture is DDS or of other commonType
	std::string fileEnding = GetFileExtension(to_string(path));
	Texture* texture = nullptr;
	if (fileEnding == "dds")
	{
		texture = new Texture2D(path);
	}
	else
	{
		texture = new Texture2DGUI(path);
	}

	m_LoadedTextures[path].first = false;
	m_LoadedTextures[path].second = texture;

	// Create dx resources etc..
	texture->Init(m_pDevice, m_pDescriptorHeap_CBV_UAV_SRV);

	return texture;
}

Shader* AssetLoader::loadShader(const std::wstring& fileName, SHADER_TYPE type)
{
	// Check if the shader already exists
	if (m_LoadedShaders.count(fileName) != 0)
	{
		return m_LoadedShaders[fileName];
	}
	// else, create a new shader and compile it

	std::wstring entireFilePath = m_FilePathShaders + fileName;
	Shader* tempShader = new Shader(entireFilePath.c_str(), type);

	m_LoadedShaders[fileName] = tempShader;
	return m_LoadedShaders[fileName];
}

void AssetLoader::processModel(const aiScene* assimpScene, std::vector<Mesh*>* meshes, std::vector<Material*>* materials, const std::wstring& filePath)
{
	for (unsigned int i = 0; i < assimpScene->mNumMeshes; i++)
	{
		aiMesh* assimpMesh = assimpScene->mMeshes[i];
		Mesh* mesh = processMesh(assimpMesh, assimpScene, meshes, materials, filePath);
		meshes->push_back(mesh);
	}
}

Mesh* AssetLoader::processMesh(aiMesh* assimpMesh, const aiScene* assimpScene, std::vector<Mesh*>* meshes, std::vector<Material*>* materials, const std::wstring& filePath)
{
	// Fill this data
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	processMeshData(assimpScene, assimpMesh, &vertices, &indices);

	// Create Mesh
	Mesh* mesh = new Mesh(
		&vertices, &indices,
		filePath);

	mesh->Init(m_pDevice, m_pDescriptorHeap_CBV_UAV_SRV);
	// save mesh
	m_LoadedMeshes.push_back(mesh);

	// add the texture to the correct mesh (later for models slotinfo)
	materials->push_back(processMaterial(filePath, assimpScene, assimpMesh));

	return mesh;
}

void AssetLoader::processMeshData(const aiScene* assimpScene, const aiMesh* assimpMesh, std::vector<Vertex>* vertices, std::vector<unsigned int>* indices)
{	
	// Get data from assimpMesh and store it
	for (unsigned int i = 0; i < assimpMesh->mNumVertices; i++)
	{
		Vertex vTemp = {};
		// Get positions
		if (assimpMesh->HasPositions())
		{
			vTemp.pos.x = assimpMesh->mVertices[i].x;
			vTemp.pos.y = assimpMesh->mVertices[i].y;
			vTemp.pos.z = assimpMesh->mVertices[i].z;
		}
		else
		{
			BL_LOG_CRITICAL("Mesh has no positions\n");
		}

		// Get Normals
		if (assimpMesh->HasNormals())
		{
			vTemp.normal.x = assimpMesh->mNormals[i].x;
			vTemp.normal.y = assimpMesh->mNormals[i].y;
			vTemp.normal.z = assimpMesh->mNormals[i].z;
		}
		else
		{
			BL_LOG_CRITICAL("Mesh has no normals\n");
		}

		// Get tangents
		if (assimpMesh->HasTangentsAndBitangents())
		{
			vTemp.tangent.x = assimpMesh->mTangents[i].x;
			vTemp.tangent.y = assimpMesh->mTangents[i].y;
			vTemp.tangent.z = assimpMesh->mTangents[i].z;
		}
		else
		{
			BL_LOG_CRITICAL("Mesh has no tangents\n");
		}

		// Get texture coordinates if there are any
		if (assimpMesh->HasTextureCoords(0))
		{
			vTemp.uv.x = (float)assimpMesh->mTextureCoords[0][i].x;
			vTemp.uv.y = (float)assimpMesh->mTextureCoords[0][i].y;
		}
		else
		{
			BL_LOG_CRITICAL("Mesh has no textureCoords\n");
		}

		vertices->push_back(vTemp);
	}

	// Get indices
	for (unsigned int i = 0; i < assimpMesh->mNumFaces; i++)
	{
		aiFace face = assimpMesh->mFaces[i];

		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			indices->push_back(face.mIndices[j]);
		}
	}
}

Material* AssetLoader::processMaterial(std::wstring path, const aiScene* assimpScene, const aiMesh* assimpMesh)
{
	// Split filepath
	std::wstring filePathWithoutTexture = path;
	std::size_t indicesInPath = filePathWithoutTexture.find_last_of(L"/\\");
	filePathWithoutTexture = filePathWithoutTexture.substr(0, indicesInPath + 1);

	// Get material from assimp
	aiMaterial* mat = assimpScene->mMaterials[assimpMesh->mMaterialIndex];
	Material* material;
	// Create our material
	material = loadMaterial(mat, filePathWithoutTexture);

	return material;			// MAY DESTRUCT CAUSE OUT OF SCOPE
}

Material* AssetLoader::loadMaterial(aiMaterial* mat, const std::wstring& folderPath)
{
	// Get material name
	aiString tempName;
	mat->Get(AI_MATKEY_NAME, tempName);
	std::wstring matName = to_wstring(tempName.C_Str());

	// Check if material don't exists
	if (m_LoadedMaterials.count(matName) == 0)
	{
		// Load material
		std::map<TEXTURE2D_TYPE, Texture*> matTextures;

		// Add the textures to the m_pMesh
		for (unsigned int i = 0; i < static_cast<unsigned int>(TEXTURE2D_TYPE::NUM_TYPES); i++)
		{
			TEXTURE2D_TYPE type = static_cast<TEXTURE2D_TYPE>(i);
			Texture* texture = processTexture(mat, type, folderPath);
			matTextures[type] = texture;
		}

		Material* material = new Material(&matName, &matTextures);
		m_LoadedMaterials[matName].first = false;
		m_LoadedMaterials[matName].second = material;

		return material;
	}
	else
	{
		// Don't print for default material
		if (matName != L"DefaultMaterial")
		{
			//Log::PrintSeverity(Log::Severity::WARNING, "AssetLoader: Loaded same material name more than once, first loaded material will be used <%S>\n", matName.c_str());
		}
		return m_LoadedMaterials[matName].second;
	}
}

Texture* AssetLoader::processTexture(aiMaterial* mat, TEXTURE2D_TYPE texture_type, const std::wstring& filePathWithoutTexture)
{
	aiTextureType type = aiTextureType::aiTextureType_NONE;
	aiString str;
	Texture* texture = nullptr;

	// incase of the texture doesn't exist
	std::wstring defaultPath = L"";
	std::string warningMessageTextureType = "";

	// Find the textureType
	switch (texture_type)
	{
	case::TEXTURE2D_TYPE::ALBEDO:
		type = aiTextureType_DIFFUSE;
		defaultPath = m_FilePathDefaultTextures + L"default_albedo.dds";
		warningMessageTextureType = "Albedo";
		break;
	case::TEXTURE2D_TYPE::ROUGHNESS:
		type = aiTextureType_SPECULAR;
		defaultPath = m_FilePathDefaultTextures + L"default_roughness.dds";
		warningMessageTextureType = "Roughness";
		break;
	case::TEXTURE2D_TYPE::METALLIC:
		type = aiTextureType_AMBIENT;
		defaultPath = m_FilePathDefaultTextures + L"default_metallic.dds";
		warningMessageTextureType = "Metallic";
		break;
	case::TEXTURE2D_TYPE::NORMAL:
		type = aiTextureType_NORMALS;
		defaultPath = m_FilePathDefaultTextures + L"default_normal.dds";
		warningMessageTextureType = "Normal";
		break;
	case::TEXTURE2D_TYPE::EMISSIVE:
		type = aiTextureType_EMISSIVE;
		defaultPath = m_FilePathDefaultTextures + L"default_emissive.dds";
		warningMessageTextureType = "Emissive";
		break;
	case::TEXTURE2D_TYPE::OPACITY:
		type = aiTextureType_OPACITY;
		defaultPath = m_FilePathDefaultTextures + L"default_opacity.dds";
		warningMessageTextureType = "Opacity";
		break;
	}

	mat->GetTexture(type, 0, &str);
	std::wstring textureFile = to_wstring(str.C_Str());
	if (textureFile.size() != 0)
	{
		texture = LoadTexture2D(filePathWithoutTexture + textureFile);
	}

	if (texture != nullptr)
	{
		return texture;
	}
	else
	{
		// Logging, avoid logging emissive and opacity
		if (texture_type == TEXTURE2D_TYPE::EMISSIVE || texture_type == TEXTURE2D_TYPE::OPACITY)
		{
			
		}
		else
		{
			std::string tempString = std::string(filePathWithoutTexture.begin(), filePathWithoutTexture.end());
			// No texture, warn and apply default Texture
			BL_LOG_WARNING("Applying default texture: " + warningMessageTextureType +
				" on mesh with path: \'%s\'\n", tempString.c_str());
		}

		return m_LoadedTextures[defaultPath].second;
	}

	return nullptr;
}
