#include "stdafx.h"
#include "PointLightComponent.h"

// Renderer
#include "../Renderer/Model/Transform.h"
#include "../Renderer/Camera/BaseCamera.h"
#include "../Renderer/Renderer.h"

// ECS
#include "../ECS/Entity.h"

namespace component
{
	PointLightComponent::PointLightComponent(Entity* parent, unsigned int lightFlags)
		:Component(parent), Light(CAMERA_TYPE::PERSPECTIVE, lightFlags)
	{
		m_pPointLight = new PointLight();
		m_pPointLight->position = { 0.0f,  2.0f,  0.0f, 0.0f };
		m_pPointLight->attenuation = { 1.0f, 0.027f, 0.0028f, 0.0f };
		m_pPointLight->baseLight = *m_pBaseLight;
	}

	PointLightComponent::~PointLightComponent()
	{
		delete m_pPointLight;
	}

	void PointLightComponent::Update(double dt)
	{
		if (m_pCamera != nullptr)
		{
			m_pCamera->Update(dt);
		}

		if (m_LightFlags & static_cast<unsigned int>(FLAG_LIGHT::USE_TRANSFORM_POSITION))
		{
			Transform* tc = m_pParent->GetComponent<TransformComponent>()->GetTransform();
			float3 position = tc->GetPositionFloat3();
			m_pPointLight->position.x = position.x;
			m_pPointLight->position.y = position.y;
			m_pPointLight->position.z = position.z;
		}
	}

	void PointLightComponent::OnInitScene()
	{
		Renderer::GetInstance().InitPointLightComponent(this);
	}

	void PointLightComponent::OnUnInitScene()
	{
		Renderer::GetInstance().UnInitPointLightComponent(this);
	}

	void PointLightComponent::SetPosition(float3 position)
	{
		m_pPointLight->position = { position.x, position.y, position.z, 1.0f };
	}

	void PointLightComponent::SetAttenuation(float3 attenuation)
	{
		m_pPointLight->attenuation.x = attenuation.x;
		m_pPointLight->attenuation.y = attenuation.y;
		m_pPointLight->attenuation.z = attenuation.z;
	}

	void* PointLightComponent::GetLightData() const
	{
		return m_pPointLight;
	}

	void PointLightComponent::UpdateLightColor()
	{
		m_pPointLight->baseLight.color = m_pBaseLight->color;
	}
}