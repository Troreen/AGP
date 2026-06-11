#include "ModelViewer.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <utility>

#include "Application.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CameraComponent.h"
#include "GameFramework/SkeletalMeshComponent.h"
#include "GameFramework/StaticMeshComponent.h"

ModelViewer::ModelViewer() = default;

bool ModelViewer::Initialize(SIZE aWindowSize, WNDPROC aWindowProcess, LPCWSTR aWindowTitle)
{
	constexpr LPCWSTR windowClassName = L"ModelViewerMainWindow";

    // First we create our Window Class
    WNDCLASS windowClass = {};
    windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = aWindowProcess;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = windowClassName;
    RegisterClass(&windowClass);

    // Put the window in the middle of the screen regardless of resolution.
    LONG posX = (GetSystemMetrics(SM_CXSCREEN) - aWindowSize.cx) / 2;
	posX = std::max<LONG>(posX, 0);

	LONG posY = (GetSystemMetrics(SM_CYSCREEN) - aWindowSize.cy) / 2;
	posY = std::max<LONG>(posY, 0);

	// Then we use the class to create our window
    myMainWindowHandle = CreateWindow(
        windowClassName,                                // Classname
        aWindowTitle,                                   // Window Title
        WS_OVERLAPPEDWINDOW | WS_POPUP,                 // Flags
        posX,
        posY,
        aWindowSize.cx,
        aWindowSize.cy,
        nullptr, nullptr, nullptr,
        nullptr
    );

    std::filesystem::path contentPath = std::filesystem::current_path() / ".." / ".." / "Assets";
	contentPath = std::filesystem::canonical(contentPath);
	myContentRoot = contentPath;

	{   // Graphics Init
        MVLOG(Log, "Initializing Graphics Engine...");

        GraphicsEngine& GE = GraphicsEngine::Get();

	    if(!GE.Initialize(myMainWindowHandle, contentPath / "Shaders"))
	        return false;

        if (!GE.CreateCommandList("Model Viewer", myCommandList))
        {
            return false;
        }
    }

    {
        myInputHandler.SetWindowHandle(myMainWindowHandle);
        myInputHandler.SetAutoMouseCapture(false);
    }

    myMeshLibrary.Initialize();

    LoadScene();

    if (myCameraActor != nullptr)
    {
        myCameraController.Init(myInputHandler, myCameraActor->GetTransform());
    }

    MVLOG(Log, "Ready!");

    // Show our program window and give it focus.
    ShowWindow(myMainWindowHandle, SW_SHOW);
    SetForegroundWindow(myMainWindowHandle);
    Application.Timer.Update();

    return true;
}

int ModelViewer::Run()
{
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));

    myIsRunning = true;

    while (myIsRunning)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            myInputHandler.UpdateEvents(msg.message, msg.wParam, msg.lParam);
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                myIsRunning = false;
            }
        }

        Application.Timer.Update();
        const float deltaTime = Application.Timer.GetDeltaTime();

        myInputHandler.UpdateInput();
        myCameraController.Update(deltaTime);
        HandleAnimationInput();
        UpdateScene(deltaTime);

        myCommandList.ResetCommandList();
        GraphicsEngine& GE = GraphicsEngine::Get();
        if (myCameraActor != nullptr)
        {
            GE.Render(myCommandList, *myCameraActor, myWorld);
            myCommandList.FinishCommandList();
            GE.ExecuteCommandList(myCommandList);
            GE.Present();
        }
        
    }

    return 0;
}

void ModelViewer::LoadScene()
{
    mySpinningActors.clear();
    myAnimatedMeshComponent = nullptr;

    {
        myCameraActor = myWorld.CreateActor("Camera Actor");
        if (myCameraActor != nullptr)
        {
            myCameraActor->AddComponent<CameraComponent>(
                "Camera",
                90.0f,
                1.0f,
                50000.0f,
                GraphicsEngine::Get().GetClientSize());

            myCameraActor->SetTranslation({ 0.0f, 100.0f, -850.0f });
            myCameraActor->LookAt({ 0.0f, 0.0f, 250.0f });
        }
    }

    if (std::shared_ptr<Mesh> axesMesh = GetRegisteredMesh("WorldAxes"))
    {
        Actor* axesActor = myWorld.CreateActor("World Axes Actor");
        if (axesActor != nullptr)
        {
            axesActor->AddComponent<StaticMeshComponent>("World Axes Mesh Component", axesMesh);
        }
    }

    if (std::shared_ptr<Mesh> chestMesh = GetRegisteredMesh("SM_Chest"))
    {
        Actor* chestActor = myWorld.CreateActor("SM_Chest Actor");
		StaticMeshComponent* chestMeshComponent = nullptr;
        if (chestActor != nullptr)
        {
            chestMeshComponent = chestActor->AddComponent<StaticMeshComponent>("SM_Chest Mesh Component", chestMesh);
            chestActor->SetTranslation({ 500.0f, -120.0f, 250.0f });
            chestActor->SetRotation(0.0f, 0.0f, 0.0f);
            chestActor->SetScale({ 1.0f, 1.0f, 1.0f });
        }

		if (chestMeshComponent != nullptr)
		{
			MaterialDescription desc;
			desc.Domain = MaterialDomain::Surface;
			desc.ShadingModel = ShadingModel::Unlit;
			desc.BlendMode = BlendMode::Opaque;
			desc.Name = "Chest_TestMaterial";
			desc.MaterialShaderCode = myContentRoot.parent_path() / "Source" / "Application" / "ModelViewer" / "Materials" / "TestMaterial.hlsli";

			std::shared_ptr<Material> material = std::make_shared<Material>();
			if (GraphicsEngine::Get().CreateMaterial(desc, *material))
			{
				for (size_t materialIndex = 0; materialIndex < chestMesh->GetNumMaterialSlots(); ++materialIndex)
				{
					chestMeshComponent->SetMaterial(static_cast<unsigned>(materialIndex), material);
				}
			}
		}
    }

    if (std::shared_ptr<Mesh> characterMesh = GetRegisteredMesh("SK_C_TGA_Bro"))
    {
        Actor* characterActor = myWorld.CreateActor("TGA Bro Actor");
        if (characterActor != nullptr)
        {
            myAnimatedMeshComponent = characterActor->AddComponent<SkeletalMeshComponent>("TGA Bro Mesh Component", characterMesh);
            characterActor->SetTranslation({ -450.0f, -120.0f, 250.0f });
            characterActor->SetRotation(180.0f, 0.0f, 0.0f);
            characterActor->SetScale({ 1.0f, 1.0f, 1.0f });

            if (myAnimatedMeshComponent != nullptr)
            {
                // TODO Engine future:
                // Replace hardcoded joint mask with data-driven animation mask assets.
                // Masks should be authored externally and resolved to joint indices when loading the skeleton.
                myAnimatedMeshComponent->ConfigurePartialLayerFromJointName("RightShoulder");
                myAnimatedMeshComponent->PlayAnimation("Breathing", true);
            }
        }
    }

    constexpr std::array primitiveNames = {
        "Plane",
        "Cube",
        "Pyramid",
        "Sphere",
        "Torus",
    };

    constexpr float spacing = 165.0f;
    const float startX = -spacing * (static_cast<float>(primitiveNames.size()) - 1.0f) * 0.5f;

    for (size_t index = 0; index < primitiveNames.size(); ++index)
    {
        const std::string primitiveName = primitiveNames[index];
        std::shared_ptr<Mesh> mesh = GetRegisteredMesh(primitiveName);
        if (mesh == nullptr)
        {
            continue;
        }

        Actor* actor = myWorld.CreateActor(primitiveName + " Actor");
        if (actor != nullptr)
        {
            actor->AddComponent<StaticMeshComponent>(primitiveName + " Mesh Component", mesh);
            actor->SetTranslation({ startX + spacing * static_cast<float>(index), 0.0f, 250.0f });
            actor->SetRotation(0.0f, 20.0f * static_cast<float>(index), 0.0f);
            actor->SetScale({ 100.0f, 100.0f, 100.0f });

        }
    }
}

std::shared_ptr<Mesh> ModelViewer::GetRegisteredMesh(const std::string& aName) const
{
    return myMeshLibrary.GetMesh(aName);
}

void ModelViewer::HandleAnimationInput()
{
    if (myAnimatedMeshComponent == nullptr)
    {
        return;
    }

    if (myInputHandler.IsKeyPressed(Keys::NUMPAD0))
    {
		myAnimatedMeshComponent->PlayAnimation("Breathing", true);
    }

    if (myInputHandler.IsKeyPressed(Keys::NUMPAD1))
    {
        myAnimatedMeshComponent->PlayAnimation("Walk", true);
    }

    if (myInputHandler.IsKeyPressed(Keys::NUMPAD2))
    {
        myAnimatedMeshComponent->PlayAnimation("Run", true);
    }

    if (myInputHandler.IsKeyPressed(Keys::NUMPAD3))
    {
        if (!myAnimatedMeshComponent->PlayPartialAnimation("Wave", false))
        {
            myAnimatedMeshComponent->PlayAnimation("Wave", false);
        }
    }
}

void ModelViewer::UpdateScene(float aDeltaTime)
{
    for (SpinningActor& spinningActor : mySpinningActors)
    {
        if (spinningActor.Instance == nullptr)
        {
            continue;
        }

        spinningActor.RotationDegrees += spinningActor.RotationSpeedDegrees * aDeltaTime;
        spinningActor.Instance->SetRotation(
            spinningActor.RotationDegrees.x,
            spinningActor.RotationDegrees.y,
            spinningActor.RotationDegrees.z);
    }

    myWorld.Update(aDeltaTime);
}
