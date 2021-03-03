#include "BeLuEngine.h"

#include <ios>

Scene* TestScene(SceneManager* sm);
Scene* SponzaScene(SceneManager* sm);

void TestUpdateScene(SceneManager* sm, double dt);
void SponzaUpdateScene(SceneManager* sm, double dt);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
#ifdef DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    /* ------ Command line arguments  ------ */
    ApplicationParameters params;
    ParseParameters(&params);


    /* ------ Engine  ------ */
    BeLuEngine engine;
    engine.Init(hInstance, nCmdShow, &params);

    /*  ------ Get references from engine  ------ */
    Window* const window = engine.GetWindow();
    Timer* const timer = engine.GetTimer();
    ThreadPool* const threadPool = engine.GetThreadPool();
    SceneManager* const sceneManager = engine.GetSceneHandler();
    Renderer* const renderer = engine.GetRenderer();
    Input* const input = &Input::GetInstance();

    

    /*------ AssetLoader to load models / textures ------*/
    AssetLoader* al = AssetLoader::Get();

    Scene* scene;
    if (params.scene == L"test")
    {
         scene = TestScene(sceneManager);
    }
    else
    {
        scene = SponzaScene(sceneManager);
    }

    // Set scene
    sceneManager->SetScene(scene);

    // Have to update models before using it in the AS buffers
    renderer->UpdateSceneToGPU();
    sceneManager->Update(0);
    renderer->InitDXR();

   BL_LOG("Entering Game-Loop ...\n\n");
   while (!window->ExitWindow())
   {
       static bool DXR = true;

       // 0: RT
       // 1: Inline RT (Pixel shader)
       // 2: inline RT (Compute shader)
       static int mode = 0;

       // Check if change mode
       bool isF1 = input->GetKeyState(SCAN_CODES::F1);
       bool isF2 = input->GetKeyState(SCAN_CODES::F2);
       bool isF3 = input->GetKeyState(SCAN_CODES::F3);

       if (isF1)
       {
           mode = 0;
           Log::Print("Mode: RT\n");
       }
       if (isF2)
       {
           mode = 1;
           Log::Print("Mode: Inline RT (Pixel shader)\n");
       }
       if (isF3)
       {
           mode = 2;
           Log::Print("Mode: inline RT (Compute shader)\n");
       }

       if (window->WasSpacePressed())
       {
           Log::Print("CamPos: x: %f, y: %f, z: %f \n", 
               sceneManager->GetActiveScene()->GetMainCamera()->GetPosition().x,
               sceneManager->GetActiveScene()->GetMainCamera()->GetPosition().y,
               sceneManager->GetActiveScene()->GetMainCamera()->GetPosition().z);
       }

       /* ------ Update ------ */
       timer->Update();
   
       sceneManager->Update(timer->GetDeltaTime());
   
       /* ------ Sort ------ */
       renderer->SortObjects();
   
       /* ------ Draw ------ */
       switch (mode)
       {
       case 0:
           renderer->ExecuteDXR();
           break;
       case 1:
           renderer->ExecuteDXRi();
           break;
       case 2:
           //renderer->ExecuteDXRi();
           Log::Print("Inline RT not yet implemented with compute shader!\n");
           break;
       default:
           Log::Print("Unknown rendering mode!!!!!!!!!!!!!!!!!!!!!!\n");
           break;
       }
   }
    
    return EXIT_SUCCESS;
}

Scene* TestScene(SceneManager* sm)
{
    // Create Scene
    Scene* scene = sm->CreateScene("TestScene");

    component::CameraComponent* cc = nullptr;
    component::ModelComponent* mc = nullptr;
    component::TransformComponent* tc = nullptr;
    component::InputComponent* ic = nullptr;
    component::BoundingBoxComponent* bbc = nullptr;
    component::PointLightComponent* plc = nullptr;
    component::DirectionalLightComponent* dlc = nullptr;
    component::SpotLightComponent* slc = nullptr;

    AssetLoader* al = AssetLoader::Get();

    // Get the models needed
    Model* sponza = al->LoadModel(L"../Vendor/Resources/Scenes/Sponza/textures_pbr/sponza.obj");
    Model* floorModel = al->LoadModel(L"../Vendor/Resources/Models/FloorPBR/floor.obj");
    Model* sphereModel = al->LoadModel(L"../Vendor/Resources/Models/SpherePBR/ball.obj");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Floor ---------------------- */
    entity = scene->AddEntity("floor");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    
    mc = entity->GetComponent<component::ModelComponent>();
    mc->SetModel(floorModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc = entity->GetComponent<component::TransformComponent>();
    tc->GetTransform()->SetScale(150, 1, 150);
    tc->GetTransform()->SetPosition(0.0f, -10.0f, 0.0f);
    /* ---------------------- Floor ---------------------- */

    entity = scene->AddEntity("sponza");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    
    mc->SetModel(sponza);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetPosition({ 0.0f, 0.0f, 0.0f });
    tc->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);

    /* ---------------------- Sphere ---------------------- */
    entity = scene->AddEntity("sphere1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    
    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(5, 15, 5);
    
    entity = scene->AddEntity("sphere2");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    
    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(5, 25, 10);
    
    entity = scene->AddEntity("sphere3");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    
    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(5, 35, 15);
    /* ---------------------- Sphere ---------------------- */

    
    //
    /* ---------------------- PointLight1 ---------------------- */
    entity = scene->AddEntity("pointLight1");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 1.0f, 0.3f, 0.3f });
    plc->SetPosition({ -26.42f, 63.0776f, 14.19f });
    /* ---------------------- PointLight1 ---------------------- */
    
    /* ---------------------- PointLight2 ---------------------- */
    entity = scene->AddEntity("pointLight2");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 1.0f });
    plc->SetPosition({ -10.895118f, 56.448929f, 150.611298f });
    /* ---------------------- PointLight2 ---------------------- */


    // Todo: Gives wierd round circles below light?? changes when modifying the lightRadius (cone width)
    /* ---------------------- PointLight3 ---------------------- */
    //entity = scene->AddEntity("pointLight3");
    //plc = entity->AddComponent<component::PointLightComponent>();
    //plc->SetColor({ 0.9f, 0.9f, 0.9f });
    //plc->SetPosition({ 346.4f, 631.f, -106.8f});
    /* ---------------------- PointLight3 ---------------------- */
    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&TestUpdateScene);
    return scene;
}

Scene* SponzaScene(SceneManager* sm)
{
    // Create Scene
    Scene* scene = sm->CreateScene("SponzaScene");

    component::CameraComponent* cc = nullptr;
    component::ModelComponent* mc = nullptr;
    component::TransformComponent* tc = nullptr;
    component::InputComponent* ic = nullptr;
    component::BoundingBoxComponent* bbc = nullptr;
    component::PointLightComponent* plc = nullptr;
    component::DirectionalLightComponent* dlc = nullptr;
    component::SpotLightComponent* slc = nullptr;

    AssetLoader* al = AssetLoader::Get();

    // Get the models needed
    Model* sponza = al->LoadModel(L"../Vendor/Resources/Scenes/Sponza/textures_pbr/sponza.obj");
    Model* floorModel = al->LoadModel(L"../Vendor/Resources/Models/FloorPBR/floor.obj");
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

    entity = scene->AddEntity("sponza1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(sponza);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetPosition({ 0.0f, 0.0f, -1000.0f });
    tc->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);
    /* ---------------------- Sponza ---------------------- */

    /* ---------------------- Floor ---------------------- */
    //entity = scene->AddEntity("floor");
    //mc = entity->AddComponent<component::ModelComponent>();
    //tc = entity->AddComponent<component::TransformComponent>();
    //
    //mc = entity->GetComponent<component::ModelComponent>();
    //mc->SetModel(floorModel);
    //mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    //tc = entity->GetComponent<component::TransformComponent>();
    //tc->GetTransform()->SetScale(1000, 1, 1000);
    //tc->GetTransform()->SetPosition(0.0f, -30.0f, 0.0f);
    /* ---------------------- Floor ---------------------- */


    /* ---------------------- Braziers ---------------------- */
    //entity = scene->AddEntity("Brazier0");
    //mc = entity->AddComponent<component::ModelComponent>();
    //tc = entity->AddComponent<component::TransformComponent>();
    //plc = entity->AddComponent<component::PointLightComponent>(FLAG_LIGHT::USE_TRANSFORM_POSITION);
    //
    //mc->SetModel(sphereModel);
    //mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    //tc->GetTransform()->SetScale(0.3f);
    //tc->GetTransform()->SetPosition({  0.0f, 10.0f, 0.0f });
    //plc->SetColor({ 0.0f, 0.0f, 15.0f });

    //entity = scene->AddEntity("Brazier1");
    //mc = entity->AddComponent<component::ModelComponent>();
    //tc = entity->AddComponent<component::TransformComponent>();
    //plc = entity->AddComponent<component::PointLightComponent>(FLAG_LIGHT::USE_TRANSFORM_POSITION);
    //
    //mc->SetModel(sphereModel);
    //mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    //tc->GetTransform()->SetScale(0.3f);
    //tc->GetTransform()->SetPosition({ -185.0f, 40.0f, -42.6f });
    //plc->SetColor({ 10.0f, 0.0f, 10.0f });
    //
    //entity = scene->AddEntity("Brazier2");
    //mc = entity->AddComponent<component::ModelComponent>();
    //tc = entity->AddComponent<component::TransformComponent>();
    //plc = entity->AddComponent<component::PointLightComponent>(FLAG_LIGHT::USE_TRANSFORM_POSITION);
    //
    //mc->SetModel(sphereModel);
    //mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    //tc->GetTransform()->SetScale(0.3f);
    //tc->GetTransform()->SetPosition({ 146.0f, 40.0f, 66.0f });
    //plc->SetColor({ 0.0f, 15.0f, 0.0f });
    //
    //entity = scene->AddEntity("Brazier3");
    //mc = entity->AddComponent<component::ModelComponent>();
    //tc = entity->AddComponent<component::TransformComponent>();
    //plc = entity->AddComponent<component::PointLightComponent>(FLAG_LIGHT::USE_TRANSFORM_POSITION);
    //
    //mc->SetModel(sphereModel);
    //mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    //tc->GetTransform()->SetScale(0.3f);
    //tc->GetTransform()->SetPosition({ 146.0f, 40.0f, -42.6f });
    //plc->SetColor({ 15.0f, 0.0f, 0.0f });
    /* ---------------------- Braziers ---------------------- */

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&SponzaUpdateScene);
    return scene;
}

void TestUpdateScene(SceneManager* sm, double dt)
{
    //static float intensity = 0.0f;
    //component::SpotLightComponent* slc = sm->GetScene("TestScene")->GetEntity("spotLightDynamic")->GetComponent<component::SpotLightComponent>();
    //float col = abs(sinf(intensity)) * 30;
    //
    //slc->SetColor({ col * 0.2f, 0.0f, col });
    //
    //intensity += 0.5f * dt;
}

void SponzaUpdateScene(SceneManager* sm, double dt)
{
    //static double a = 0.0f;
    //a += 0.001f;
    //sm->GetScene("SponzaScene")->GetEntity("sponza")->GetComponent<component::TransformComponent>()->GetTransform()->SetRotationZ(abs(sinf(a)));
}
