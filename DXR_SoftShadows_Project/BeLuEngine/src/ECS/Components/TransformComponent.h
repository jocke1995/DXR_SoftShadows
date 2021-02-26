#ifndef TRANSFORMCOMPONENT_H
#define TRANSFORMCOMPONENT_H

#include "Component.h"
class Transform;

class DescriptorHeap;

struct ID3D12Device5;

class Resource;

namespace component
{
    class TransformComponent : public Component
    {
    public:
        TransformComponent(Entity* parent, bool invertDirection = false);
        virtual ~TransformComponent();

        void Update(double dt) override;
        void OnInitScene() override;
        void OnUnInitScene() override;
        // Resets the transform to its original state
        void Reset() override;

        void SetTransformOriginalState();

        Transform* GetTransform() const;
        Resource* GetMatrixUploadResource() const;

        void CreateResourceForWorldMatrix(ID3D12Device5* device, DescriptorHeap* descriptorHeap_CBV_UAV_SRV);
    private:
        Transform* m_pTransform = nullptr;
        Transform* m_pOriginalTransform = nullptr;

        Resource* m_pResourceWorldMatrixUpload = nullptr;
    };
}

#endif

