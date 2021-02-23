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

		this->c = {};
	}

	TransformComponent::~TransformComponent()
	{
		delete m_pTransform;
		delete m_pOriginalTransform;
		delete m_pTempCB;

		delete m_pResourceWorldMatrixUpload;
	}

	void TransformComponent::Update(double dt)
	{
		m_pTransform->UpdateWorldMatrix();
		m_pResourceWorldMatrixUpload->SetData(m_pTransform->GetWorldMatrixTransposed());

		// Temp for testing
		static double a = 0.0f;
		a += 0.001f;

		int id = m_pParent->GetID();
		if (id == 1)
		{
			this->c.col = { abs(sinf(a)), 0.0f, abs(sinf(a - 0.3f)), 1.0f };
			this->m_pTempCB->GetUploadResource()->SetData(&this->c);
		}
		else if(id == 2)
		{
			this->c.col = { 0, abs(sinf(a)), abs(sinf(a)), 1.0f };
			this->m_pTempCB->GetUploadResource()->SetData(&this->c);
		}
		//else
		//{
		//	this->c.col = { 0, abs(sinf(a)), 0, 1.0f };
		//	this->m_pTempCB->GetUploadResource()->SetData(&this->c);
		//}
		
		
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

		// Temp for testing
		m_pTempCB = new ConstantBuffer(device, sizeof(COLOR_TEMP_STRUCT), L"COLOR_TEMP_STRUCT_PER_INSTANCE", descriptorHeap_CBV_UAV_SRV);
	}
}
