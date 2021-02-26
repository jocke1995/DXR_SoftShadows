#include "stdafx.h"
#include "TransformComponent.h"
#include "../../Renderer/Model/Transform.h"
#include "../ECS/Entity.h"

#include "Core.h"

#include "../Renderer/GPUMemory/GPUMemory.h"

namespace component
{
	TransformComponent::TransformComponent(Entity* parent, bool invertDirection)
		:Component(parent)
	{
		m_pTransform = new Transform(invertDirection);
		m_pOriginalTransform = new Transform(invertDirection);
	}

	TransformComponent::~TransformComponent()
	{
		delete m_pTransform;
		delete m_pOriginalTransform;

		delete m_pResourceWorldMatrixUpload;
	}

	void TransformComponent::Update(double dt)
	{
		m_pTransform->UpdateWorldMatrix();
		m_pResourceWorldMatrixUpload->SetData(m_pTransform->GetWorldMatrixTransposed());
	}

	void TransformComponent::OnInitScene()
	{
	}

	void TransformComponent::OnUnInitScene()
	{
	}

	void TransformComponent::Reset()
	{
		*m_pTransform = *m_pOriginalTransform;
	}

	void TransformComponent::SetTransformOriginalState()
	{
		*m_pOriginalTransform = *m_pTransform;
	}

	Transform* TransformComponent::GetTransform() const
	{
		return m_pTransform;
	}

	Resource* TransformComponent::GetMatrixUploadResource() const
	{
		return m_pResourceWorldMatrixUpload;
	}

	void TransformComponent::CreateResourceForWorldMatrix(ID3D12Device5* device, DescriptorHeap* descriptorHeap_CBV_UAV_SRV)
	{
		std::wstring resourceName = to_wstring(m_pParent->GetName()) + L"_WORLD_MATRIX_UPLOAD";
		m_pResourceWorldMatrixUpload = new Resource(device, sizeof(DirectX::XMMATRIX), RESOURCE_TYPE::UPLOAD, resourceName);
	}
}
