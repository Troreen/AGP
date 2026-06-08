#include "ModelViewer.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <utility>

#include "Application.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CameraComponent.h"
#include "GameFramework/MeshComponent.h"
#include "PrimitiveMeshBuilder.h"

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

	{   // Graphics Init
        MVLOG(Log, "Initializing Graphics Engine...");

        GraphicsEngine& GE = GraphicsEngine::Get();

	    if(!GE.Initialize(myMainWindowHandle))
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

    RegisterPrimitiveMeshes();

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
            actor->AddComponent<MeshComponent>(primitiveName + " Mesh Component", mesh);
            actor->SetTranslation({ startX + spacing * static_cast<float>(index), 0.0f, 250.0f });
            actor->SetRotation(0.0f, 20.0f * static_cast<float>(index), 0.0f);
            actor->SetScale({ 100.0f, 100.0f, 100.0f });

            SpinningActor spinner;
            spinner.Instance = actor;
            spinner.RotationDegrees = { 0.0f, 20.0f * static_cast<float>(index), 0.0f };
            spinner.RotationSpeedDegrees = {
                10.0f + 2.0f * static_cast<float>(index),
                18.0f + 4.0f * static_cast<float>(index),
                6.0f + static_cast<float>(index)
            };
            mySpinningActors.push_back(spinner);
        }
    }
}

void ModelViewer::RegisterPrimitiveMeshes()
{
    RegisterMesh("Plane", PrimitiveMeshBuilder::CreatePlane());
    RegisterMesh("Cube", PrimitiveMeshBuilder::CreateCube());
    RegisterMesh("Pyramid", PrimitiveMeshBuilder::CreatePyramid());
    RegisterMesh("Sphere", PrimitiveMeshBuilder::CreateSphere());
    RegisterMesh("Torus", PrimitiveMeshBuilder::CreateTorus());
}

void ModelViewer::RegisterMesh(std::string aName, std::shared_ptr<Mesh> aMesh)
{
    if (aName.empty() || aMesh == nullptr)
    {
        return;
    }

    myMeshRegistry[std::move(aName)] = std::move(aMesh);
}

std::shared_ptr<Mesh> ModelViewer::GetRegisteredMesh(const std::string& aName) const
{
    const auto foundMesh = myMeshRegistry.find(aName);
    if (foundMesh == myMeshRegistry.end())
    {
        return nullptr;
    }

    return foundMesh->second;
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
