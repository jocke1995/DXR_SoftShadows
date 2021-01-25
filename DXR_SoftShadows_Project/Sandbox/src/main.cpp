#include "BeLuEngine.h"

#include <ios>

Scene* TestScene(SceneManager* sm);
Scene* SponzaScene(SceneManager* sm);

void TestUpdateScene(SceneManager* sm, double dt);
void SponzaUpdateScene(SceneManager* sm, double dt);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    
    /* ------ Command line arguments  ------ */
    ApplicationParameters params;
    ParseParameters(&params);


    CSVExporter exporter;

    /*
    exporter <<
        "Film:År:Distributör" << std::endl << 
        "Reservoir Dogs:1992:Miramax" << std::endl <<
        "Pulp Fiction : 1994 : Miramax" << std::endl <<
        "Jackie Brown : 1997 : Miramax" << std::endl <<
        "Kill Bill: Volume 1 : 2003 : Miramax" << std::endl <<
        "Kill Bill: Volume 2 : 2004 : Miramax" << std::endl <<
        "Death Proof : 2007 : Dimension Films" << std::endl <<
        "Inglourious Basterds : 2009 : Universal Pictures" << std::endl <<
        "Django Unchained : 2012 : Sony Pictures Releasing" << std::endl <<
        "The Hateful Eight : 2015 : The Weinstein Company" << std::endl <<
        "Once Upon A Time in Hollywood : 2019 : Sony Pictures" << std::endl;

        */

    exporter <<
        "Film:År:Distributör" << std::endl <<
        "Reservoir Dogs:1992:Miramax" << std::endl <<
        "Pulp Fiction:1994:Miramax" << std::endl <<
        "Jackie Brown:1997:Miramax" << std::endl <<
        "Kill Bill:Volume 1:2003:Miramax" << std::endl <<
        "Kill Bill:Volume 2:2004:Miramax" << std::endl <<
        "Death Proof:2007:Dimension Films" << std::endl <<
        "Inglourious Basterds:2009:Universal Pictures" << std::endl <<
        "Django Unchained:2012:Sony Pictures Releasing" << std::endl <<
        "The Hateful Eight:2015:The Weinstein Company" << std::endl <<
        "Once Upon A Time in Hollywood:2019:Sony Pictures" << std::endl;
        

    exporter.Export();

    exporter.Print();





    /* ------ Engine  ------ */
    BeLuEngine engine;
    engine.Init(hInstance, nCmdShow, &params);

    /*  ------ Get references from engine  ------ */
    Window* const window = engine.GetWindow();
    Timer* const timer = engine.GetTimer();
    ThreadPool* const threadPool = engine.GetThreadPool();
    SceneManager* const sceneManager = engine.GetSceneHandler();
    Renderer* const renderer = engine.GetRenderer();

    /*------ AssetLoader to load models / textures ------*/
   AssetLoader* al = AssetLoader::Get();
   
   Scene* scene;
   if (params.scene == L"test2")
   {
        scene = TestScene(sceneManager);
   }
   else
   {
       scene = SponzaScene(sceneManager);
   }
   
   
   // Set scene
   sceneManager->SetScene(scene);

   
   

   
   Log::Print("Entering Game-Loop ...\n\n");
   while (!window->ExitWindow())
   {
       /* ------ Update ------ */
       timer->Update();
   
       sceneManager->Update(timer->GetDeltaTime());
   
       /* ------ Sort ------ */
       renderer->SortObjects();
   
       /* ------ Draw ------ */
       renderer->Execute();
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
    tc->GetTransform()->SetScale(50, 1, 50);
    tc->GetTransform()->SetPosition(0.0f, 0.0f, 0.0f);
    /* ---------------------- Floor ---------------------- */

     /* ---------------------- Sphere ---------------------- */
    entity = scene->AddEntity("sphere");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(0, 4, 30);
    /* ---------------------- Sphere ---------------------- */

    /* ---------------------- dirLight ---------------------- */
    entity = scene->AddEntity("dirLight");
    dlc = entity->AddComponent<component::DirectionalLightComponent>(FLAG_LIGHT::CAST_SHADOW);
    dlc->SetColor({ 0.6f, 0.6f, 0.6f });
    dlc->SetDirection({ 1.0f, -1.0f, 0.0f });
    dlc->SetCameraTop(30.0f);
    dlc->SetCameraBot(-30.0f);
    dlc->SetCameraLeft(-70.0f);
    dlc->SetCameraRight(70.0f);
    /* ---------------------- dirLight ---------------------- */

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
    tc->GetTransform()->SetPosition({0.0f, 0.0f, 0.0f});
    tc->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);
    /* ---------------------- Sponza ---------------------- */

    /* ---------------------- Braziers ---------------------- */
    entity = scene->AddEntity("Brazier0");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(FLAG_LIGHT::USE_TRANSFORM_POSITION);

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(0.3f);
    tc->GetTransform()->SetPosition({ -185.0f, 40.0f, 66.0f });
    plc->SetColor({ 0.0f, 0.0f, 15.0f });

    entity = scene->AddEntity("Brazier1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(FLAG_LIGHT::USE_TRANSFORM_POSITION);

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(0.3f);
    tc->GetTransform()->SetPosition({ -185.0f, 40.0f, -42.6f });
    plc->SetColor({ 10.0f, 0.0f, 10.0f });

    entity = scene->AddEntity("Brazier2");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(FLAG_LIGHT::USE_TRANSFORM_POSITION);

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(0.3f);
    tc->GetTransform()->SetPosition({ 146.0f, 40.0f, 66.0f });
    plc->SetColor({ 0.0f, 15.0f, 0.0f });

    entity = scene->AddEntity("Brazier3");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(FLAG_LIGHT::USE_TRANSFORM_POSITION);

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE);
    tc->GetTransform()->SetScale(0.3f);
    tc->GetTransform()->SetPosition({ 146.0f, 40.0f, -42.6f });
    plc->SetColor({ 15.0f, 0.0f, 0.0f });
    /* ---------------------- Braziers ---------------------- */

    /* ---------------------- dirLight ---------------------- */
    entity = scene->AddEntity("dirLight");
    dlc = entity->AddComponent<component::DirectionalLightComponent>(FLAG_LIGHT::CAST_SHADOW);
    dlc->SetColor({ 0.17, 0.25, 0.3f});
    dlc->SetCameraDistance(300);
    dlc->SetDirection({ -1.0f, -2.0f, 0.03f });
    dlc->SetCameraTop(800.0f);
    dlc->SetCameraBot(-550.0f);
    dlc->SetCameraLeft(-550.0f);
    dlc->SetCameraRight(550.0f);
    dlc->SetCameraFarZ(5000);
    /* ---------------------- dirLight ---------------------- */

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

}
