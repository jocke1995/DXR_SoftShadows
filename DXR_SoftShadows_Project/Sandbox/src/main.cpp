#include "BeLuEngine.h"
#include "TestScenes.h"

#include <ios>

Scene* TestScene(SceneManager* sm);

void TestUpdateScene(SceneManager* sm, double dt);

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
    if (params.scene == L"teast")
    {
         scene = TestScene(sceneManager);
    }
    else
    {
        scene = SponzaScene3(sceneManager);
    }

    // Set scene
    sceneManager->SetScene(scene);

    // Have to update models before using it in the AS buffers
    renderer->UpdateSceneToGPU();
    sceneManager->Update(0);
    renderer->InitDXR();

    // 0: RT
    // 1: Inline RT (Pixel shader)
    // 2: inline RT (Compute shader)
    static int mode = 0;

    if (params.RayTracingType == L"rt")
    {
        mode = 0;
    }
    else if (params.RayTracingType == L"ip")
    {
        mode = 1;
    }
    else if (params.RayTracingType == L"ic")
    {
        mode = 2;
    }

    Log::Print("Start Mode: %d\n", mode);

    BL_LOG("Entering Game-Loop ...\n\n");
    while (!window->ExitWindow())
    {
        // Check if change mode
        bool isF1 = input->GetKeyState(SCAN_CODES::F1);
        bool isF2 = input->GetKeyState(SCAN_CODES::F2);
        bool isF3 = input->GetKeyState(SCAN_CODES::F3);

        if (isF1)
        {
            if (mode != 0)
                Log::Print("Mode: RT\n");
            mode = 0;
        }
        if (isF2)
        {
            if (mode != 1)
                Log::Print("Mode: IP\n");
            mode = 1;
        }
        if (isF3)
        {
            if (mode != 2)
                Log::Print("Mode: IC\n");
            mode = 2;
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
            renderer->ExecuteDXR(timer->GetDeltaTime());
            break;
        case 1:
            renderer->ExecuteInlinePixel(timer->GetDeltaTime());
            break;
        case 2:
            renderer->ExecuteInlineCompute(timer->GetDeltaTime());
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


    entity = scene->AddEntity("sponza");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    
    mc->SetModel(sponza);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetPosition({ 0.0f, 0.0f, 0.0f });
    tc->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);

    /* ---------------------- Spheres ---------------------- */
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
    /* ---------------------- Spheres ---------------------- */

    /* ---------------------- PointLight1 ---------------------- */
    entity = scene->AddEntity("pointLight1");
    plc = entity->AddComponent<component::PointLightComponent>();
    plc->SetColor({ 0.3f, 0.3f, 0.3f });
    plc->SetPosition({ -26.42f, 63.0776f, 14.19f });
    /* ---------------------- PointLight1 ---------------------- */
    

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&TestUpdateScene);
    return scene;
}

void TestUpdateScene(SceneManager* sm, double dt)
{
    
}
