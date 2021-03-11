#include "TestScenes.h"

#include "GameIncludes.h"

void EmptyUpdateScene(SceneManager* sm, double dt)
{
}

/*
A figure of each scene will represent how the lights are positioned in Sponza
L = Light
|, _, [, ] = walls
*/

/*
1 Light in the middle of the scene
__________
[  |   |  ]
[  |   |  ]
[  |   |  ]
[  | L |  ]
[  |   |  ]
[  |   |  ]
[__|___|__]

*/
Scene* SponzaScene1(SceneManager* sm)
{
    // Create Scene
    Scene* scene = sm->CreateScene("SponzaScene");

    component::CameraComponent* cc = nullptr;
    component::InputComponent* ic = nullptr;
    component::ModelComponent* mc = nullptr;
    component::TransformComponent* tc = nullptr;
    component::PointLightComponent* plc = nullptr;

    AssetLoader* al = AssetLoader::Get();

    // Get the models needed
    Model* sponza = al->LoadModel(L"../Vendor/Resources/Scenes/Sponza/textures_pbr/sponza.obj");
    Model* sphereModel = al->LoadModel(L"../Vendor/Resources/Models/SpherePBR/ball.obj");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Sponza ---------------------- */
    entity = scene->AddEntity("sponza");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(sponza);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetPosition({ 0.0f, 0.0f, 0.0f });
    tc->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);
    /* ---------------------- Sponza ---------------------- */

    /* ---------------------- PointLight1 ---------------------- */
    entity = scene->AddEntity("pointLight1");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -26.42f, 63.0776f, 14.19f });
    /* ---------------------- PointLight1 ---------------------- */

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&EmptyUpdateScene);
    return scene;
}

/*
2 Lights in the middle of the scene
__________
[  |   |  ]
[  | L |  ]
[  |   |  ]
[  |   |  ]
[  |   |  ]
[  | L |  ]
[__|___|__]

*/
Scene* SponzaScene2(SceneManager* sm)
{
    // Create Scene
    Scene* scene = sm->CreateScene("SponzaScene");

    component::CameraComponent* cc = nullptr;
    component::InputComponent* ic = nullptr;
    component::ModelComponent* mc = nullptr;
    component::TransformComponent* tc = nullptr;
    component::PointLightComponent* plc = nullptr;

    AssetLoader* al = AssetLoader::Get();

    // Get the models needed
    Model* sponza = al->LoadModel(L"../Vendor/Resources/Scenes/Sponza/textures_pbr/sponza.obj");
    Model* sphereModel = al->LoadModel(L"../Vendor/Resources/Models/SpherePBR/ball.obj");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Sponza ---------------------- */
    entity = scene->AddEntity("sponza");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(sponza);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetPosition({ 0.0f, 0.0f, 0.0f });
    tc->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);
    /* ---------------------- Sponza ---------------------- */

    /* ---------------------- PointLight1 ---------------------- */
    entity = scene->AddEntity("pointLight1");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -187.561005, 63.0776f, 14.19f });
    /* ---------------------- PointLight1 ---------------------- */

    /* ---------------------- PointLight2 ---------------------- */
    entity = scene->AddEntity("pointLight2");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ 187.561005, 63.0776f, 14.19f });
    /* ---------------------- PointLight2 ---------------------- */

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&EmptyUpdateScene);
    return scene;
}

/*
4 Lights, same 2 from SponzaScene2, but adding 2 more lights on each side of the curtains
____________
[   |   |   ]
[   | L |   ]
[   |   |   ]
[ L |   | L ]
[   |   |   ]
[   | L |   ]
[___|___|___]

*/
Scene* SponzaScene3(SceneManager* sm)
{
    // Create Scene
    Scene* scene = sm->CreateScene("SponzaScene");

    component::CameraComponent* cc = nullptr;
    component::InputComponent* ic = nullptr;
    component::ModelComponent* mc = nullptr;
    component::TransformComponent* tc = nullptr;
    component::PointLightComponent* plc = nullptr;

    AssetLoader* al = AssetLoader::Get();

    // Get the models needed
    Model* sponza = al->LoadModel(L"../Vendor/Resources/Scenes/Sponza/textures_pbr/sponza.obj");
    Model* sphereModel = al->LoadModel(L"../Vendor/Resources/Models/SpherePBR/ball.obj");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Sponza ---------------------- */
    entity = scene->AddEntity("sponza");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(sponza);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetPosition({ 0.0f, 0.0f, 0.0f });
    tc->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);
    /* ---------------------- Sponza ---------------------- */
    
    /* ---------------------- PointLight1 ---------------------- */
    entity = scene->AddEntity("pointLight1");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -187.561005, 63.0776f, 14.19f });
    /* ---------------------- PointLight1 ---------------------- */

    /* ---------------------- PointLight2 ---------------------- */
    entity = scene->AddEntity("pointLight2");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ 187.561005, 63.0776f, 14.19f });
    /* ---------------------- PointLight2 ---------------------- */

    /* ---------------------- PointLight3 ---------------------- */
    entity = scene->AddEntity("pointLight3");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -26.42f, 63.0776f, -110.8f });
    /* ---------------------- PointLight3 ---------------------- */

    /* ---------------------- PointLight4 ---------------------- */
    entity = scene->AddEntity("pointLight4");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -26.42f, 63.0776f, 140.26f });
    /* ---------------------- PointLight4 ---------------------- */

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&EmptyUpdateScene);
    return scene;
}

/*
8 Lights, 3 in the middle (near floor), 1 in the middle (higher up), 2 on each side of the curtains
____________
[   |   |   ]
[ L | L | L ]
[   |   |   ]
[   | L |   ]  // One more here, but higher up
[   |   |   ]
[ L | L | L ]
[___|___|___]

*/
Scene* SponzaScene4(SceneManager* sm)
{
    // Create Scene
    Scene* scene = sm->CreateScene("SponzaScene");

    component::CameraComponent* cc = nullptr;
    component::InputComponent* ic = nullptr;
    component::ModelComponent* mc = nullptr;
    component::TransformComponent* tc = nullptr;
    component::PointLightComponent* plc = nullptr;

    AssetLoader* al = AssetLoader::Get();

    // Get the models needed
    Model* sponza = al->LoadModel(L"../Vendor/Resources/Scenes/Sponza/textures_pbr/sponza.obj");
    Model* sphereModel = al->LoadModel(L"../Vendor/Resources/Models/SpherePBR/ball.obj");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Sponza ---------------------- */
    entity = scene->AddEntity("sponza");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(sponza);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetPosition({ 0.0f, 0.0f, 0.0f });
    tc->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);
    /* ---------------------- Sponza ---------------------- */

    /* ---------------------- PointLight1 ---------------------- */
    entity = scene->AddEntity("pointLight1");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -20.4f, 52.1f, 11.8f });
    /* ---------------------- PointLight1 ---------------------- */

    /* ---------------------- PointLight2 ---------------------- */
    entity = scene->AddEntity("pointLight2");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -243.4f, 52.1f, 11.8f });
    /* ---------------------- PointLight2 ---------------------- */

    /* ---------------------- PointLight3 ---------------------- */
    entity = scene->AddEntity("pointLight3");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ 201.6f, 52.1f, 11.8f });
    /* ---------------------- PointLight3 ---------------------- */

    /* ---------------------- PointLight4 ---------------------- */
    entity = scene->AddEntity("pointLight4");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ 196.3f, 52.1f, -123.1f });
    /* ---------------------- PointLight4 ---------------------- */

    /* ---------------------- PointLight5 ---------------------- */
    entity = scene->AddEntity("pointLight5");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ 196.3f, 52.1f, 143.6f });
    /* ---------------------- PointLight5 ---------------------- */

     /* ---------------------- PointLight6 ---------------------- */
    entity = scene->AddEntity("pointLight6");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -245.8f, 52.1f, 143.6f });
    /* ---------------------- PointLight6 ---------------------- */

     /* ---------------------- PointLight7 ---------------------- */
    entity = scene->AddEntity("pointLight7");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -245.8f, 52.1f, -123.1f });
    /* ---------------------- PointLight7 ---------------------- */

     /* ---------------------- PointLight8 ---------------------- */
    entity = scene->AddEntity("pointLight8");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -20.4f, 252.1f, 11.8f });
    /* ---------------------- PointLight8 ---------------------- */

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&EmptyUpdateScene);
    return scene;
}


/*
1 Light in the middle of the scene
__________
[  |   |  ]
[  |   |  ]
[  |   |  ]
[  | L |  ]
[  |   |  ]
[  |   |  ]
[__|___|__]

*/
Scene* SponzaDragonsScene1(SceneManager* sm)
{
    // Create Scene
    Scene* scene = sm->CreateScene("SponzaScene");

    component::CameraComponent* cc = nullptr;
    component::InputComponent* ic = nullptr;
    component::ModelComponent* mc = nullptr;
    component::TransformComponent* tc = nullptr;
    component::PointLightComponent* plc = nullptr;

    AssetLoader* al = AssetLoader::Get();

    // Get the models needed
    Model* dragonModel = al->LoadModel(L"../Vendor/Resources/Models/StanfordDragon/drag.obj");
    Model* sponza = al->LoadModel(L"../Vendor/Resources/Scenes/Sponza/textures_pbr/sponza.obj");
    Model* sphereModel = al->LoadModel(L"../Vendor/Resources/Models/SpherePBR/ball.obj");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Sponza ---------------------- */
    entity = scene->AddEntity("sponza");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(sponza);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetPosition({ 0.0f, 0.0f, 0.0f });
    tc->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);
    /* ---------------------- Sponza ---------------------- */

    /* ---------------------- Stanford Dragons ---------------------- */
    entity = scene->AddEntity("dragon1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(143.3f, 10.0f, 0.0f);

    entity = scene->AddEntity("dragon2");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(29.6f, 10.0f, 0.0f);

    entity = scene->AddEntity("dragon3");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(-77.2f, 10.0f, 0.0f);

    entity = scene->AddEntity("dragon4");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(-188.9f, 10.0f, 0.0f);
    /* ---------------------- Stanford Dragons ---------------------- */

    /* ---------------------- PointLight1 ---------------------- */
    entity = scene->AddEntity("pointLight1");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -26.42f, 63.0776f, 14.19f });
    /* ---------------------- PointLight1 ---------------------- */

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&EmptyUpdateScene);
    return scene;
}

/*
2 Lights in the middle of the scene
__________
[  |   |  ]
[  | L |  ]
[  |   |  ]
[  |   |  ]
[  |   |  ]
[  | L |  ]
[__|___|__]

*/
Scene* SponzaDragonsScene2(SceneManager* sm)
{
    // Create Scene
    Scene* scene = sm->CreateScene("SponzaScene");

    component::CameraComponent* cc = nullptr;
    component::InputComponent* ic = nullptr;
    component::ModelComponent* mc = nullptr;
    component::TransformComponent* tc = nullptr;
    component::PointLightComponent* plc = nullptr;

    AssetLoader* al = AssetLoader::Get();

    // Get the models needed
    Model* dragonModel = al->LoadModel(L"../Vendor/Resources/Models/StanfordDragon/drag.obj");
    Model* sponza = al->LoadModel(L"../Vendor/Resources/Scenes/Sponza/textures_pbr/sponza.obj");
    Model* sphereModel = al->LoadModel(L"../Vendor/Resources/Models/SpherePBR/ball.obj");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Sponza ---------------------- */
    entity = scene->AddEntity("sponza");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(sponza);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetPosition({ 0.0f, 0.0f, 0.0f });
    tc->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);
    /* ---------------------- Sponza ---------------------- */

    /* ---------------------- Stanford Dragons ---------------------- */
    entity = scene->AddEntity("dragon1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(143.3f, 10.0f, 0.0f);

    entity = scene->AddEntity("dragon2");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(29.6f, 10.0f, 0.0f);

    entity = scene->AddEntity("dragon3");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(-77.2f, 10.0f, 0.0f);

    entity = scene->AddEntity("dragon4");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(-188.9f, 10.0f, 0.0f);
    /* ---------------------- Stanford Dragons ---------------------- */

    /* ---------------------- PointLight1 ---------------------- */
    entity = scene->AddEntity("pointLight1");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -187.561005, 63.0776f, 14.19f });
    /* ---------------------- PointLight1 ---------------------- */

    /* ---------------------- PointLight2 ---------------------- */
    entity = scene->AddEntity("pointLight2");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ 187.561005, 63.0776f, 14.19f });
    /* ---------------------- PointLight2 ---------------------- */

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&EmptyUpdateScene);
    return scene;
}

/*
4 Lights, same 2 from SponzaScene2, but adding 2 more lights on each side of the curtains
____________
[   |   |   ]
[   | L |   ]
[   |   |   ]
[ L |   | L ]
[   |   |   ]
[   | L |   ]
[___|___|___]

*/
Scene* SponzaDragonsScene3(SceneManager* sm)
{
    // Create Scene
    Scene* scene = sm->CreateScene("SponzaScene");

    component::CameraComponent* cc = nullptr;
    component::InputComponent* ic = nullptr;
    component::ModelComponent* mc = nullptr;
    component::TransformComponent* tc = nullptr;
    component::PointLightComponent* plc = nullptr;

    AssetLoader* al = AssetLoader::Get();

    // Get the models needed
    Model* dragonModel = al->LoadModel(L"../Vendor/Resources/Models/StanfordDragon/drag.obj");
    Model* sponza = al->LoadModel(L"../Vendor/Resources/Scenes/Sponza/textures_pbr/sponza.obj");
    Model* sphereModel = al->LoadModel(L"../Vendor/Resources/Models/SpherePBR/ball.obj");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Sponza ---------------------- */
    entity = scene->AddEntity("sponza");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(sponza);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetPosition({ 0.0f, 0.0f, 0.0f });
    tc->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);
    /* ---------------------- Sponza ---------------------- */

    /* ---------------------- Stanford Dragons ---------------------- */
    entity = scene->AddEntity("dragon1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(143.3f, 10.0f, 0.0f);

    entity = scene->AddEntity("dragon2");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(29.6f, 10.0f, 0.0f);

    entity = scene->AddEntity("dragon3");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(-77.2f, 10.0f, 0.0f);

    entity = scene->AddEntity("dragon4");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(-188.9f, 10.0f, 0.0f);
    /* ---------------------- Stanford Dragons ---------------------- */

    /* ---------------------- PointLight1 ---------------------- */
    entity = scene->AddEntity("pointLight1");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -187.561005, 63.0776f, 14.19f });
    /* ---------------------- PointLight1 ---------------------- */

    /* ---------------------- PointLight2 ---------------------- */
    entity = scene->AddEntity("pointLight2");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ 187.561005, 63.0776f, 14.19f });
    /* ---------------------- PointLight2 ---------------------- */

    /* ---------------------- PointLight3 ---------------------- */
    entity = scene->AddEntity("pointLight3");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -26.42f, 63.0776f, -110.8f });
    /* ---------------------- PointLight3 ---------------------- */

    /* ---------------------- PointLight4 ---------------------- */
    entity = scene->AddEntity("pointLight4");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -26.42f, 63.0776f, 140.26f });
    /* ---------------------- PointLight4 ---------------------- */

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&EmptyUpdateScene);
    return scene;
}

/*
8 Lights, 3 in the middle (near floor), 1 in the middle (higher up), 2 on each side of the curtains
____________
[   |   |   ]
[ L | L | L ]
[   |   |   ]
[   | L |   ]  // One more here, but higher up
[   |   |   ]
[ L | L | L ]
[___|___|___]

*/
Scene* SponzaDragonsScene4(SceneManager* sm)
{
    // Create Scene
    Scene* scene = sm->CreateScene("SponzaScene");

    component::CameraComponent* cc = nullptr;
    component::InputComponent* ic = nullptr;
    component::ModelComponent* mc = nullptr;
    component::TransformComponent* tc = nullptr;
    component::PointLightComponent* plc = nullptr;

    AssetLoader* al = AssetLoader::Get();

    // Get the models needed
    Model* dragonModel = al->LoadModel(L"../Vendor/Resources/Models/StanfordDragon/drag.obj");
    Model* sponza = al->LoadModel(L"../Vendor/Resources/Scenes/Sponza/textures_pbr/sponza.obj");
    Model* sphereModel = al->LoadModel(L"../Vendor/Resources/Models/SpherePBR/ball.obj");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Sponza ---------------------- */
    entity = scene->AddEntity("sponza");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(sponza);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetPosition({ 0.0f, 0.0f, 0.0f });
    tc->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);
    /* ---------------------- Sponza ---------------------- */

    /* ---------------------- Stanford Dragons ---------------------- */
    entity = scene->AddEntity("dragon1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(143.3f, 10.0f, 0.0f);

    entity = scene->AddEntity("dragon2");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(29.6f, 10.0f, 0.0f);

    entity = scene->AddEntity("dragon3");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(-77.2f, 10.0f, 0.0f);

    entity = scene->AddEntity("dragon4");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(dragonModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(250.0f);
    tc->GetTransform()->SetRotationY(PI / 2);
    tc->GetTransform()->SetRotationX(-PI / 2);
    tc->GetTransform()->SetPosition(-188.9f, 10.0f, 0.0f);
    /* ---------------------- Stanford Dragons ---------------------- */

    /* ---------------------- PointLight1 ---------------------- */
    entity = scene->AddEntity("pointLight1");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -20.4f, 52.1f, 11.8f });
    /* ---------------------- PointLight1 ---------------------- */

    /* ---------------------- PointLight2 ---------------------- */
    entity = scene->AddEntity("pointLight2");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -243.4f, 52.1f, 11.8f });
    /* ---------------------- PointLight2 ---------------------- */

    /* ---------------------- PointLight3 ---------------------- */
    entity = scene->AddEntity("pointLight3");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ 201.6f, 52.1f, 11.8f });
    /* ---------------------- PointLight3 ---------------------- */

    /* ---------------------- PointLight4 ---------------------- */
    entity = scene->AddEntity("pointLight4");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ 196.3f, 52.1f, -123.1f });
    /* ---------------------- PointLight4 ---------------------- */

    /* ---------------------- PointLight5 ---------------------- */
    entity = scene->AddEntity("pointLight5");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ 196.3f, 52.1f, 143.6f });
    /* ---------------------- PointLight5 ---------------------- */

     /* ---------------------- PointLight6 ---------------------- */
    entity = scene->AddEntity("pointLight6");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -245.8f, 52.1f, 143.6f });
    /* ---------------------- PointLight6 ---------------------- */

     /* ---------------------- PointLight7 ---------------------- */
    entity = scene->AddEntity("pointLight7");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -245.8f, 52.1f, -123.1f });
    /* ---------------------- PointLight7 ---------------------- */

     /* ---------------------- PointLight8 ---------------------- */
    entity = scene->AddEntity("pointLight8");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -20.4f, 252.1f, 11.8f });
    /* ---------------------- PointLight8 ---------------------- */

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&EmptyUpdateScene);
    return scene;
}
