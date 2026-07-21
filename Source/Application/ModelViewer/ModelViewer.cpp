#include "ModelViewer.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "Application.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CameraComponent.h"
#include "GameFramework/LightComponent.h"
#include "GameFramework/SkeletalMeshComponent.h"
#include "GameFramework/StaticMeshComponent.h"
#include "GraphicsEngine/Materials/Material.h"

ModelViewer::ModelViewer() = default;

namespace
{
	using Vector3f = CommonUtilities::Vector3f;
	using namespace std::chrono_literals;

	constexpr float FixedUpdateDeltaTime = 1.0f / 60.0f;
	constexpr float MaxFrameDeltaTime = 0.25f;
	constexpr int MaxFixedStepsPerFrame = 5;
	constexpr int KeyCount = 256;

	std::filesystem::path GetExecutableDirectory()
	{
		wchar_t executablePath[MAX_PATH] = {};
		const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
		if (pathLength == 0 || pathLength == MAX_PATH)
		{
			return {};
		}

		return std::filesystem::path(executablePath).parent_path();
	}

	std::filesystem::path GetContentRoot()
	{
		const std::filesystem::path executableDirectory = GetExecutableDirectory();
		if (executableDirectory.empty())
		{
			return {};
		}

		const std::filesystem::path contentRoot = executableDirectory / ".." / ".." / "Assets";
		std::error_code error;
		if (!std::filesystem::is_directory(contentRoot, error))
		{
			return {};
		}

		return std::filesystem::canonical(contentRoot, error);
	}

	std::filesystem::path GetMaterialRoot(const std::filesystem::path& aContentRoot)
	{
		return aContentRoot.parent_path() / "Source" / "Application" / "ModelViewer" / "Materials";
	}

	void AssignMaterialToAllSlots(MeshComponentBase* aMeshComponent, const std::shared_ptr<MaterialInterface>& aMaterial)
	{
		if (aMeshComponent == nullptr || aMaterial == nullptr || !aMeshComponent->HasMesh())
		{
			return;
		}

		const std::shared_ptr<Mesh> mesh = aMeshComponent->GetMesh();
		for (size_t materialIndex = 0; materialIndex < mesh->GetNumMaterialSlots(); ++materialIndex)
		{
			aMeshComponent->SetMaterial(static_cast<unsigned>(materialIndex), aMaterial);
		}
	}

	bool IsVirtualKeyDown(int aVirtualKey)
	{
		return (GetAsyncKeyState(aVirtualKey) & 0x8000) != 0;
	}

	bool IsValidKeyIndex(int aKeyCode)
	{
		return aKeyCode >= 0 && aKeyCode < KeyCount;
	}

	void AimActorAlongCameraForward(Actor& anActor, const CommonUtilities::Transform& aCameraTransform)
	{
		Vector3f forward = aCameraTransform.GetForward();
		if (forward.LengthSqr() <= 0.0f)
		{
			forward = Vector3f::UnitZ;
		}
		else
		{
			forward.Normalize();
		}

		const Vector3f position = anActor.GetTransform().GetPosition();
		anActor.LookAt(position + forward);
	}

	void PrintLightTuningValues(
		const DirectionalLightComponent* aDirectionalLightComponent,
		const std::vector<PointLightComponent*>& somePointLightComponents,
		const SpotLightComponent* aSpotLightComponent)
	{
		unsigned activeLightCount = 0;
		if (aDirectionalLightComponent != nullptr)
		{
			activeLightCount += aDirectionalLightComponent->IsEnabled() ? 1 : 0;
			const Vector3f direction = aDirectionalLightComponent->GetWorldDirection();
			MVLOG(Log, "Directional light direction: {{ {:.2f}, {:.2f}, {:.2f} }}, intensity: {:.2f}",
				direction.x, direction.y, direction.z, aDirectionalLightComponent->GetIntensity());
		}

		for (size_t pointIndex = 0; pointIndex < somePointLightComponents.size(); ++pointIndex)
		{
			const PointLightComponent* pointLightComponent = somePointLightComponents[pointIndex];
			if (pointLightComponent == nullptr)
			{
				continue;
			}

			activeLightCount += pointLightComponent->IsEnabled() ? 1 : 0;
			const Vector3f position = pointLightComponent->GetWorldPosition();
			MVLOG(Log, "Point light {} position: {{ {:.2f}, {:.2f}, {:.2f} }}, intensity: {:.2f}, radius: {:.2f}",
				pointIndex, position.x, position.y, position.z, pointLightComponent->GetIntensity(), pointLightComponent->GetRadius());
		}

		if (aSpotLightComponent != nullptr)
		{
			activeLightCount += aSpotLightComponent->IsEnabled() ? 1 : 0;
			const Vector3f position = aSpotLightComponent->GetWorldPosition();
			const Vector3f direction = aSpotLightComponent->GetWorldDirection();
			MVLOG(Log, "Spot light position: {{ {:.2f}, {:.2f}, {:.2f} }}, direction: {{ {:.2f}, {:.2f}, {:.2f} }}, intensity: {:.2f}, radius: {:.2f}",
				position.x, position.y, position.z,
				direction.x, direction.y, direction.z,
				aSpotLightComponent->GetIntensity(), aSpotLightComponent->GetRadius());
		}

		MVLOG(Log, "Active demo lights: {}", activeLightCount);
		GraphicsEngine::Get().LogShadowTuning();
	}

	PointLightComponent* CreatePointLight(
		World& aWorld,
		const char* aName,
		const Vector3f& aPosition,
		const Vector3f& aColor,
		float anIntensity,
		float aRadius)
	{
		Actor* pointLightActor = aWorld.CreateActor(std::string(aName) + " Actor");
		if (pointLightActor == nullptr)
		{
			return nullptr;
		}

		pointLightActor->SetTranslation(aPosition);
		PointLightComponent* pointLightComponent = pointLightActor->AddComponent<PointLightComponent>(std::string(aName) + " Light");
		if (pointLightComponent != nullptr)
		{
			pointLightComponent->SetColor(aColor);
			pointLightComponent->SetIntensity(anIntensity);
			pointLightComponent->SetRadius(aRadius);
		}

		return pointLightComponent;
	}
}

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

	const std::filesystem::path contentPath = GetContentRoot();
	if (contentPath.empty())
	{
		MVLOG(Error, "Could not locate the Assets directory from the executable or working directory.");
		return false;
	}
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

	myMeshLibrary.Initialize(myContentRoot);

    LoadScene();

    if (myCameraActor != nullptr)
    {
        myCameraController.Init(myCameraActor->GetTransform());
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
    // After this point, the update thread owns World/component mutation.
    // The main thread only pumps Win32 input and renders immutable snapshots.
    StartUpdateThread();

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

        myInputHandler.UpdateInput();
        SubmitInputFrame(CaptureInputFrame());

        GraphicsEngine& GE = GraphicsEngine::Get();
        myCommandList.ResetCommandList();
        if (const GraphicsEngine::RenderSceneSnapshot* snapshot = myRenderSnapshots.AcquireLatest())
        {
            GE.RenderSnapshot(myCommandList, *snapshot);
            myCommandList.FinishCommandList();
            GE.ExecuteCommandList(myCommandList);
            GE.Present();
        }
        
    }

    StopUpdateThread();
    myRenderSnapshots.ReleaseRendering();

    return 0;
}

void ModelViewer::ModelViewerInputFrame::ClearPressed()
{
    KeysPressed.fill(false);
    CameraInput.MouseDeltaX = 0.0f;
    CameraInput.MouseDeltaY = 0.0f;
}

bool ModelViewer::IsKeyDown(const ModelViewerInputFrame& anInputFrame, Keys aKey)
{
    const int keyCode = static_cast<int>(aKey);
    return IsValidKeyIndex(keyCode) && anInputFrame.KeysDown[static_cast<size_t>(keyCode)];
}

bool ModelViewer::IsKeyPressed(const ModelViewerInputFrame& anInputFrame, Keys aKey)
{
    const int keyCode = static_cast<int>(aKey);
    return IsValidKeyIndex(keyCode) && anInputFrame.KeysPressed[static_cast<size_t>(keyCode)];
}

ModelViewer::ModelViewerInputFrame ModelViewer::CaptureInputFrame()
{
    ModelViewerInputFrame inputFrame;
    const bool isFocused = myMainWindowHandle != nullptr && GetForegroundWindow() == myMainWindowHandle;

    for (int keyCode = 0; keyCode < KeyCount; ++keyCode)
    {
        inputFrame.KeysDown[static_cast<size_t>(keyCode)] =
            myInputHandler.IsKeyDown(keyCode) || (isFocused && IsVirtualKeyDown(keyCode));
        inputFrame.KeysPressed[static_cast<size_t>(keyCode)] = myInputHandler.IsKeyPressed(keyCode);
    }

    const bool rightMouseDown = inputFrame.KeysDown[static_cast<size_t>(Keys::MOUSERBUTTON)];
    if (isFocused && rightMouseDown)
    {
        RECT clientRect = {};
        if (GetClientRect(myMainWindowHandle, &clientRect) != 0)
        {
            const POINT centerPoint = {
                (clientRect.right - clientRect.left) / 2,
                (clientRect.bottom - clientRect.top) / 2
            };

            inputFrame.CameraInput.MouseLookActive = true;
            if (myHasMainThreadMouseLookAnchor)
            {
                POINT mousePosScreen = {};
                GetCursorPos(&mousePosScreen);
                POINT mousePosClient = mousePosScreen;
                ScreenToClient(myMainWindowHandle, &mousePosClient);
                inputFrame.CameraInput.MouseDeltaX = static_cast<float>(mousePosClient.x - centerPoint.x);
                inputFrame.CameraInput.MouseDeltaY = static_cast<float>(mousePosClient.y - centerPoint.y);
            }

            POINT centerPointScreen = centerPoint;
            ClientToScreen(myMainWindowHandle, &centerPointScreen);
            SetCursorPos(centerPointScreen.x, centerPointScreen.y);
            myHasMainThreadMouseLookAnchor = true;
        }
    }
    else
    {
        myHasMainThreadMouseLookAnchor = false;
    }

    inputFrame.CameraInput.MoveForward = IsKeyDown(inputFrame, Keys::W);
    inputFrame.CameraInput.MoveBackward = IsKeyDown(inputFrame, Keys::S);
    inputFrame.CameraInput.MoveRight = IsKeyDown(inputFrame, Keys::D);
    inputFrame.CameraInput.MoveLeft = IsKeyDown(inputFrame, Keys::A);
    inputFrame.CameraInput.MoveUp = IsKeyDown(inputFrame, Keys::SPACE);
    inputFrame.CameraInput.MoveDown = IsKeyDown(inputFrame, Keys::CONTROL);
    return inputFrame;
}

void ModelViewer::SubmitInputFrame(const ModelViewerInputFrame& anInputFrame)
{
    {
        std::scoped_lock lock(myInputMutex);
        if (!myHasPendingInputFrame)
        {
            myPendingInputFrame = anInputFrame;
            myHasPendingInputFrame = true;
        }
        else
        {
            const float accumulatedMouseDeltaX = myPendingInputFrame.CameraInput.MouseDeltaX + anInputFrame.CameraInput.MouseDeltaX;
            const float accumulatedMouseDeltaY = myPendingInputFrame.CameraInput.MouseDeltaY + anInputFrame.CameraInput.MouseDeltaY;

            for (size_t keyIndex = 0; keyIndex < myPendingInputFrame.KeysPressed.size(); ++keyIndex)
            {
                myPendingInputFrame.KeysPressed[keyIndex] = myPendingInputFrame.KeysPressed[keyIndex] || anInputFrame.KeysPressed[keyIndex];
            }

            myPendingInputFrame.KeysDown = anInputFrame.KeysDown;
            myPendingInputFrame.CameraInput = anInputFrame.CameraInput;
            myPendingInputFrame.CameraInput.MouseDeltaX = accumulatedMouseDeltaX;
            myPendingInputFrame.CameraInput.MouseDeltaY = accumulatedMouseDeltaY;
            myPendingInputFrame.CameraInput.MouseLookActive =
                anInputFrame.CameraInput.MouseLookActive || accumulatedMouseDeltaX != 0.0f || accumulatedMouseDeltaY != 0.0f;
        }
    }

    myInputCondition.notify_one();
}

bool ModelViewer::ConsumePendingInputFrame(ModelViewerInputFrame& inoutInputFrame)
{
    std::scoped_lock lock(myInputMutex);
    if (!myHasPendingInputFrame)
    {
        return false;
    }

    const float accumulatedMouseDeltaX =
        inoutInputFrame.CameraInput.MouseDeltaX + myPendingInputFrame.CameraInput.MouseDeltaX;
    const float accumulatedMouseDeltaY =
        inoutInputFrame.CameraInput.MouseDeltaY + myPendingInputFrame.CameraInput.MouseDeltaY;

    for (size_t keyIndex = 0; keyIndex < inoutInputFrame.KeysPressed.size(); ++keyIndex)
    {
        inoutInputFrame.KeysPressed[keyIndex] =
            inoutInputFrame.KeysPressed[keyIndex] || myPendingInputFrame.KeysPressed[keyIndex];
    }

    inoutInputFrame.KeysDown = myPendingInputFrame.KeysDown;
    inoutInputFrame.CameraInput = myPendingInputFrame.CameraInput;
    inoutInputFrame.CameraInput.MouseDeltaX = accumulatedMouseDeltaX;
    inoutInputFrame.CameraInput.MouseDeltaY = accumulatedMouseDeltaY;
    inoutInputFrame.CameraInput.MouseLookActive =
        myPendingInputFrame.CameraInput.MouseLookActive
        || accumulatedMouseDeltaX != 0.0f
        || accumulatedMouseDeltaY != 0.0f;

    myPendingInputFrame = {};
    myHasPendingInputFrame = false;
    return true;
}

void ModelViewer::StartUpdateThread()
{
    StopUpdateThread();

    myRenderSnapshots.Reset();

    {
        std::scoped_lock lock(myInputMutex);
        myPendingInputFrame = {};
        myHasPendingInputFrame = false;
    }

    myHasMainThreadMouseLookAnchor = false;
    myUpdateWorker.Start(
        EngineScheduling::FixedStepUpdateWorker<ModelViewerInputFrame>::Config{
            .FixedDeltaTime = FixedUpdateDeltaTime,
            .MaxFrameDeltaTime = MaxFrameDeltaTime,
            .MaxFixedStepsPerWake = MaxFixedStepsPerFrame
        },
        [this](ModelViewerInputFrame& inoutInputFrame)
        {
            return ConsumePendingInputFrame(inoutInputFrame);
        },
        [this](float aDeltaTime, ModelViewerInputFrame& inoutInputFrame)
        {
            RunFixedUpdateStep(aDeltaTime, inoutInputFrame);
        },
        [this]()
        {
            BuildAndPublishRenderSnapshot();
        },
        [this](std::stop_token aStopToken)
        {
            std::unique_lock lock(myInputMutex);
            myInputCondition.wait_for(lock, 1ms, [this, &aStopToken]
            {
                return aStopToken.stop_requested() || myHasPendingInputFrame;
            });
        });
}

void ModelViewer::StopUpdateThread()
{
    myInputCondition.notify_all();
    myUpdateWorker.Stop();
}

void ModelViewer::RunFixedUpdateStep(float aDeltaTime, ModelViewerInputFrame& inoutInputFrame)
{
    myCameraController.Update(aDeltaTime, inoutInputFrame.CameraInput);
    HandleAnimationInput(inoutInputFrame);
    HandleLightInput(inoutInputFrame);
    UpdateScene(aDeltaTime);
}

void ModelViewer::BuildAndPublishRenderSnapshot()
{
    if (myCameraActor == nullptr)
    {
        return;
    }

    GraphicsEngine::RenderSceneSnapshot* snapshot = myRenderSnapshots.BeginBuild();
    if (snapshot == nullptr)
    {
        return;
    }

    if (GraphicsEngine::Get().BuildRenderSnapshot(*myCameraActor, myWorld, *snapshot))
    {
        myRenderSnapshots.Publish(snapshot);
    }
    else
    {
        myRenderSnapshots.CancelBuild(snapshot);
    }
}

void ModelViewer::LogRuntimeStats() const
{
    const GraphicsEngine::RenderStats renderStats = GraphicsEngine::Get().GetLastRenderStats();
    const auto snapshotStats = myRenderSnapshots.GetStats();

    MVLOG(Log, "Render stats: meshes visible {}/{}, shadow casters {}, lights relevant {}/{}, shadow passes D/S/P = {}/{}/{}",
        renderStats.VisibleRenderItems,
        renderStats.TotalRenderItems,
        renderStats.ShadowCasters,
        renderStats.RelevantLights,
        renderStats.TotalLights,
        renderStats.DirectionalShadowPasses,
        renderStats.SpotShadowPasses,
        renderStats.PointShadowPasses);
    MVLOG(Log, "Shadow culling/threading: caster draws {}, culled per pass {}, command lists recorded/executed {}/{}",
        renderStats.ShadowCasterDraws,
        renderStats.CulledShadowCasters,
        renderStats.ShadowCommandListsRecorded,
        renderStats.ShadowCommandListsExecuted);
    MVLOG(Log, "Snapshot worker: fixed ticks {}, published {}, reused previous {}, dropped ready {}",
        myUpdateWorker.GetTickCount(),
        snapshotStats.PublishedSnapshots,
        snapshotStats.ReusedSnapshots,
        snapshotStats.DroppedReadySnapshots);
}

void ModelViewer::LoadScene()
{
    mySpinningActors.clear();
    myAnimatedMeshComponent = nullptr;
    myDirectionalLightComponent = nullptr;
    myPointLightComponents.clear();
    mySpotLightComponent = nullptr;
    const std::filesystem::path materialRoot = GetMaterialRoot(myContentRoot);
    const Vector3f sceneFocus = { 25.0f, 0.0f, 260.0f };
    const Vector3f floorPosition = { 0.0f, 0.0f, 260.0f };
    const Vector3f characterPosition = { 0.0f, 0.0f, 250.0f };
    const Vector3f chestPosition = { 135.0f, 0.0f, 285.0f };
    const Vector3f colorCheckerPosition = { -145.0f, 40.0f, 365.0f };
    const Vector3f smoothSpherePosition = { 320.0f, 100.0f, 430.0f };

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

            myCameraActor->SetTranslation({ 0.0f, 260.0f, -950.0f });
            myCameraActor->LookAt(sceneFocus);
        }
    }

    {
        Actor* directionalLightActor = myWorld.CreateActor("Directional Light Actor");
        if (directionalLightActor != nullptr)
        {
            directionalLightActor->SetTranslation({ -450.0f, 650.0f, -350.0f });
            directionalLightActor->LookAt(sceneFocus);
            myDirectionalLightComponent = directionalLightActor->AddComponent<DirectionalLightComponent>("Directional Light");
            if (myDirectionalLightComponent != nullptr)
            {
                myDirectionalLightComponent->SetColor({ 1.0f, 0.96f, 0.9f });
                myDirectionalLightComponent->SetIntensity(11.0f);
            }
        }

        myPointLightComponents.push_back(CreatePointLight(myWorld, "Warm Character Point", { -90.0f, 180.0f, 150.0f }, { 1.0f, 0.42f, 0.22f }, 420.0f, 760.0f));
        //myPointLightComponents.push_back(CreatePointLight(myWorld, "Cool Character Point", { 60.0f, 75.0f, 330.0f }, { 0.25f, 0.55f, 1.0f }, 500.0f, 620.0f));
        //myPointLightComponents.push_back(CreatePointLight(myWorld, "Chest Accent Point", { 170.0f, 25.0f, 235.0f }, { 0.35f, 1.0f, 0.55f }, 560.0f, 560.0f));

        Actor* spotLightActor = myWorld.CreateActor("Spot Light Actor");
        if (spotLightActor != nullptr)
        {
            spotLightActor->SetTranslation({ 430.0f, 430.0f, -210.0f });
            spotLightActor->LookAt(sceneFocus);
            mySpotLightComponent = spotLightActor->AddComponent<SpotLightComponent>("Spot Light");
            if (mySpotLightComponent != nullptr)
            {
                mySpotLightComponent->SetColor({ 0.55f, 0.7f, 1.0f });
                mySpotLightComponent->SetIntensity(2800.0f);
                mySpotLightComponent->SetRadius(1200.0f);
                mySpotLightComponent->SetConeAnglesDegrees(18.0f, 34.0f);
            }
        }
    }

    CreateStaticMeshActor("Floor Actor", "Floor Mesh Component", "Floor",
        materialRoot / "FloorMaterial.mat",
        floorPosition,
        { 0.0f, -90.0f, 0.0f },
        { 1100.0f, 1100.0f, 1100.0f });

    CreateStaticMeshActor("SM_Chest Actor", "SM_Chest Mesh Component", "SM_Chest",
        materialRoot / "ChestMaterial.mat",
        chestPosition,
        { -18.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f });

    CreateStaticMeshActor("SM_Color_Checker Actor", "SM_Color_Checker Mesh Component", "SM_Color_Checker",
        materialRoot / "ColorCheckerMaterial.mat",
        colorCheckerPosition,
        { 0.0f, -90.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f });

    if (std::shared_ptr<Mesh> characterMesh = GetRegisteredMesh("SK_C_TGA_Bro"))
    {
        Actor* characterActor = myWorld.CreateActor("TGA Bro Actor");
        if (characterActor != nullptr)
        {
            myAnimatedMeshComponent = characterActor->AddComponent<SkeletalMeshComponent>("TGA Bro Mesh Component", characterMesh);
            characterActor->SetTranslation(characterPosition);
            characterActor->SetRotation(180.0f, 0.0f, 0.0f);
            characterActor->SetScale({ 1.0f, 1.0f, 1.0f });

            if (myAnimatedMeshComponent != nullptr)
            {
                AssignMaterialToAllSlots(myAnimatedMeshComponent, GetMaterial(materialRoot / "CharacterMaterial.mat"));

                // TODO Engine future:
                // Replace hardcoded joint mask with data-driven animation mask assets.
                // Masks should be authored externally and resolved to joint indices when loading the skeleton.
                myAnimatedMeshComponent->ConfigurePartialLayerFromJointName("RightShoulder");
                myAnimatedMeshComponent->PlayAnimation("Breathing", true);
            }
        }
    }

    if (BenchmarkScene::IsBusyScenario())
    {
        const std::array placements = BenchmarkScene::BuildBusyPrimitivePlacements();
        for (size_t placementIndex = 0; placementIndex < placements.size(); ++placementIndex)
        {
            const BenchmarkScene::PrimitivePlacement& placement = placements[placementIndex];
            const std::string actorName = "Benchmark Primitive " + std::to_string(placementIndex);
            CreateStaticMeshActor(
                actorName,
                actorName + " Mesh Component",
                placement.MeshName,
                materialRoot / "FloorMaterial.mat",
                placement.Position,
                placement.RotationDegrees,
                placement.Scale);
        }
        MVLOG(Log, "Loaded deterministic busy benchmark scene with {} primitive actors.", placements.size());
    }
}

std::shared_ptr<MaterialInterface> ModelViewer::GetMaterial(const std::filesystem::path& aMaterialFile)
{
    const std::string cacheKey = aMaterialFile.lexically_normal().string();
    if (const auto materialIt = myMaterialCache.find(cacheKey); materialIt != myMaterialCache.end())
    {
        return materialIt->second;
    }

    MaterialDescription description;
    if (!LoadMaterialDescription(aMaterialFile, description))
    {
        MVLOG(Warning, "Could not load material description '{}'.", aMaterialFile.string());
        return nullptr;
    }

    std::shared_ptr<Material> material = std::make_shared<Material>();
    if (!GraphicsEngine::Get().CreateMaterial(description, *material))
    {
        MVLOG(Warning, "Could not create material '{}'.", description.Name);
        return nullptr;
    }

    const auto materialResult = myMaterialCache.emplace(cacheKey, material);
    return materialResult.first->second;
}

StaticMeshComponent* ModelViewer::CreateStaticMeshActor(
    const std::string& anActorName,
    const std::string& aComponentName,
    const std::string& aMeshName,
    const std::filesystem::path& aMaterialFile,
    const CommonUtilities::Vector3<float>& aPosition,
    const CommonUtilities::Vector3<float>& aRotationDegrees,
    const CommonUtilities::Vector3<float>& aScale)
{
    const std::shared_ptr<Mesh> mesh = GetRegisteredMesh(aMeshName);
    if (mesh == nullptr)
    {
        MVLOG(Warning, "Scene mesh '{}' is not registered.", aMeshName);
        return nullptr;
    }

    Actor* actor = myWorld.CreateActor(anActorName);
    if (actor == nullptr)
    {
        return nullptr;
    }

    StaticMeshComponent* meshComponent = actor->AddComponent<StaticMeshComponent>(aComponentName, mesh);
    actor->SetTranslation(aPosition);
    actor->SetRotation(aRotationDegrees.x, aRotationDegrees.y, aRotationDegrees.z);
    actor->SetScale(aScale);

    AssignMaterialToAllSlots(meshComponent, GetMaterial(aMaterialFile));
    return meshComponent;
}

std::shared_ptr<Mesh> ModelViewer::GetRegisteredMesh(const std::string& aName) const
{
    return myMeshLibrary.GetMesh(aName);
}

void ModelViewer::HandleAnimationInput(const ModelViewerInputFrame& anInputFrame)
{
    if (myAnimatedMeshComponent == nullptr)
    {
        return;
    }

    if (IsKeyPressed(anInputFrame, Keys::NUMPAD0))
    {
		myAnimatedMeshComponent->PlayAnimation("Breathing", true);
    }

    if (IsKeyPressed(anInputFrame, Keys::NUMPAD1))
    {
        myAnimatedMeshComponent->PlayAnimation("Walk", true);
    }

    if (IsKeyPressed(anInputFrame, Keys::NUMPAD2))
    {
        myAnimatedMeshComponent->PlayAnimation("Run", true);
    }

    if (IsKeyPressed(anInputFrame, Keys::NUMPAD3))
    {
        if (!myAnimatedMeshComponent->PlayPartialAnimation("Wave", false))
        {
            myAnimatedMeshComponent->PlayAnimation("Wave", false);
        }
    }
}

void ModelViewer::HandleLightInput(const ModelViewerInputFrame& anInputFrame)
{
    const bool shiftDown =
        IsKeyDown(anInputFrame, Keys::SHIFT)
        || IsKeyDown(anInputFrame, Keys::LSHIFT)
        || IsKeyDown(anInputFrame, Keys::RSHIFT);
    GraphicsEngine& graphicsEngine = GraphicsEngine::Get();

    if (IsKeyPressed(anInputFrame, Keys::F5))
    {
        graphicsEngine.ResetShadowTuning();
    }

    if (IsKeyPressed(anInputFrame, Keys::F6))
    {
        graphicsEngine.AdjustShadowBias(LightType::Directional, -0.00005f);
    }

    if (IsKeyPressed(anInputFrame, Keys::F7))
    {
        graphicsEngine.AdjustShadowBias(LightType::Directional, 0.00005f);
    }

    if (IsKeyPressed(anInputFrame, Keys::F8))
    {
        graphicsEngine.AdjustShadowBias(LightType::Spot, -0.00002f);
    }

    if (IsKeyPressed(anInputFrame, Keys::F9))
    {
        graphicsEngine.AdjustShadowBias(LightType::Spot, 0.00002f);
    }

    if (IsKeyPressed(anInputFrame, Keys::F10))
    {
        graphicsEngine.AdjustShadowBias(LightType::Point, -0.00005f);
    }

    if (IsKeyPressed(anInputFrame, Keys::F11))
    {
        graphicsEngine.AdjustShadowBias(LightType::Point, 0.00005f);
    }

    if (IsKeyPressed(anInputFrame, Keys::P))
    {
        PrintLightTuningValues(myDirectionalLightComponent, myPointLightComponents, mySpotLightComponent);
        LogRuntimeStats();
    }

    if (shiftDown && myCameraActor != nullptr)
    {
        const CommonUtilities::Transform& cameraTransform = myCameraActor->GetTransform();
        const Vector3f cameraPosition = cameraTransform.GetPosition();

        if ((IsKeyPressed(anInputFrame, Keys::NUMPAD7) || anInputFrame.KeysPressed[static_cast<size_t>('7')])
            && myDirectionalLightComponent != nullptr)
        {
            if (Actor* lightActor = myDirectionalLightComponent->GetOwner())
            {
                AimActorAlongCameraForward(*lightActor, cameraTransform);
                const Vector3f direction = myDirectionalLightComponent->GetWorldDirection();
                MVLOG(Log, "Aimed directional light from camera direction: {{ {:.2f}, {:.2f}, {:.2f} }}",
                    direction.x, direction.y, direction.z);
            }
            return;
        }

        if (IsKeyPressed(anInputFrame, Keys::NUMPAD8) || anInputFrame.KeysPressed[static_cast<size_t>('8')])
        {
            for (PointLightComponent* pointLightComponent : myPointLightComponents)
            {
                if (pointLightComponent == nullptr)
                {
                    continue;
                }

                if (Actor* lightActor = pointLightComponent->GetOwner())
                {
                    lightActor->SetPosition(cameraPosition);
                    MVLOG(Log, "Moved point light to camera position: {{ {:.2f}, {:.2f}, {:.2f} }}",
                        cameraPosition.x, cameraPosition.y, cameraPosition.z);
                    break;
                }
            }
            return;
        }

        if ((IsKeyPressed(anInputFrame, Keys::NUMPAD9) || anInputFrame.KeysPressed[static_cast<size_t>('9')])
            && mySpotLightComponent != nullptr)
        {
            if (Actor* lightActor = mySpotLightComponent->GetOwner())
            {
                lightActor->SetPosition(cameraPosition);
                AimActorAlongCameraForward(*lightActor, cameraTransform);
                const Vector3f direction = mySpotLightComponent->GetWorldDirection();
                MVLOG(Log, "Moved spot light to camera and aimed forward. Position: {{ {:.2f}, {:.2f}, {:.2f} }}, direction: {{ {:.2f}, {:.2f}, {:.2f} }}",
                    cameraPosition.x, cameraPosition.y, cameraPosition.z,
                    direction.x, direction.y, direction.z);
            }
            return;
        }
    }

    if ((IsKeyPressed(anInputFrame, Keys::NUMPAD7) || anInputFrame.KeysPressed[static_cast<size_t>('7')])
        && myDirectionalLightComponent != nullptr)
    {
        myDirectionalLightComponent->SetEnabled(!myDirectionalLightComponent->IsEnabled());
    }

    if (IsKeyPressed(anInputFrame, Keys::NUMPAD8) || anInputFrame.KeysPressed[static_cast<size_t>('8')])
    {
        bool shouldEnable = true;
        bool foundPointLight = false;
        for (const PointLightComponent* pointLightComponent : myPointLightComponents)
        {
            if (pointLightComponent != nullptr)
            {
                shouldEnable = !pointLightComponent->IsEnabled();
                foundPointLight = true;
                break;
            }
        }

        if (foundPointLight)
        {
            for (PointLightComponent* pointLightComponent : myPointLightComponents)
            {
                if (pointLightComponent != nullptr)
                {
                    pointLightComponent->SetEnabled(shouldEnable);
                }
            }
        }
    }

    if ((IsKeyPressed(anInputFrame, Keys::NUMPAD9) || anInputFrame.KeysPressed[static_cast<size_t>('9')])
        && mySpotLightComponent != nullptr)
    {
        mySpotLightComponent->SetEnabled(!mySpotLightComponent->IsEnabled());
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
