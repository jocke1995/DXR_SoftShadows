#ifndef GAMEINCLUDES_H
#define GAMEINCLUDES_H

// Miscellaneous
#include "Misc/Window.h"
#include "Misc/Timer.h"
#include "Misc/AssetLoader.h"
#include "Misc/ApplicationParameters.h"
#include "Misc/CSVExporter.h"

// Entity Component System
#include "ECS/SceneManager.h"
#include "ECS/Scene.h"
#include "ECS/Entity.h"
#include "ECS/Components/ModelComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/Lights/DirectionalLightComponent.h"
#include "ECS/Components/Lights/PointLightComponent.h"
#include "ECS/Components/Lights/SpotLightComponent.h"
#include "ECS/Components/CameraComponent.h"

// Sub-engines
#include "Renderer/Renderer.h"
#include "Renderer/Model/Transform.h"
#include "Renderer/Model/Mesh.h"
#include "Renderer/Camera/BaseCamera.h"

// Textures
#include "Renderer/Model/Material.h"
#include "Renderer/Texture/TextureCubeMap.h"
#include "Renderer/Texture/Texture2DGUI.h"

// Event-handling
#include "Events/EventBus.h"

// Input
#include "Input/Input.h"

#endif