#include "stdafx.h"
#include "ModelComponent.h"

#include "../Renderer/Model/Model.h"
#include "../Renderer/Model/Material.h"
#include "../Renderer/Renderer.h"
#include "../Entity.h"

namespace component
{
	ModelComponent::ModelComponent(Entity* parent)
		:Component(parent)
	{
	}

	ModelComponent::~ModelComponent()
	{
		
	}

	void ModelComponent::SetModel(Model* model)
	{
		m_pModel = model;
	}

	void ModelComponent::SetDrawFlag(unsigned int drawFlag)
	{
		m_DrawFlag = drawFlag;
	}

	void ModelComponent::Update(double dt)
	{
	}

	void ModelComponent::OnInitScene()
	{
		Renderer::GetInstance().InitModelComponent(this);
	}

	void ModelComponent::OnUnInitScene()
	{
		Renderer::GetInstance().UnInitModelComponent(this);
	}

	Mesh* ModelComponent::GetMeshAt(unsigned int index) const
	{
		return m_pModel->GetMeshAt(index);
	}

	Material* ModelComponent::GetMaterialAt(unsigned int index) const
	{
		return m_pModel->GetMaterialAt(index);
	}

	const SlotInfo* ModelComponent::GetSlotInfoAt(unsigned int index) const
	{
		return m_pModel->GetSlotInfoAt(index);
	}

	unsigned int ModelComponent::GetDrawFlag() const
	{
		return m_DrawFlag;
	}

	unsigned int ModelComponent::GetNrOfMeshes() const
	{
		return m_pModel->GetSize();
	}

	const std::wstring& ModelComponent::GetModelPath() const
	{
		return m_pModel->GetPath();
	}

	bool ModelComponent::IsPickedThisFrame() const
	{
		return m_IsPickedThisFrame;
	}

	Model* ModelComponent::GetModel() const
	{
		return m_pModel;
	}
}
