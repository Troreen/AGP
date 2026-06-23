#include "ModelViewer.h"

#include <algorithm>
#include <array>
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

	struct PrimitivePlacement
	{
		const char* Name = "";
		const char* MaterialName = "";
		Vector3f Position;
		Vector3f RotationDegrees;
		Vector3f Scale;
	};

	std::filesystem::path GetMaterialRoot(const std::filesystem::path& aContentRoot)
	{
		return aContentRoot.parent_path() / "Source" / "Application" / "ModelViewer" / "Materials";
	}

	std::shared_ptr<Material> CreateMaterialFromFile(const std::filesystem::path& aMaterialFile)
	{
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

		return material;
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

	bool IsShiftDown(const CommonUtilities::InputHandler& anInputHandler)
	{
		return anInputHandler.IsKeyDown(Keys::SHIFT)
			|| anInputHandler.IsKeyDown(Keys::LSHIFT)
			|| anInputHandler.IsKeyDown(Keys::RSHIFT);
	}

	bool IsKeyPressed(const CommonUtilities::InputHandler& anInputHandler, Keys aNumpadKey, char aDigitKey)
	{
		return anInputHandler.IsKeyPressed(aNumpadKey)
			|| anInputHandler.IsKeyPressed(static_cast<int>(aDigitKey));
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

    std::filesystem::path contentPath = std::filesystem::current_path() / ".." / ".." / ".." / "Assets";
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
        HandleLightInput();
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
    myDirectionalLightComponent = nullptr;
    myPointLightComponents.clear();
    mySpotLightComponent = nullptr;
    const std::filesystem::path materialRoot = GetMaterialRoot(myContentRoot);
    const Vector3f sceneFocus = { 25.0f, 0.0f, 260.0f };
    const Vector3f floorPosition = { 0.0f, 0.0f, 260.0f };
    const Vector3f characterPosition = { 0.0f, 0.0f, 250.0f };
    const Vector3f chestPosition = { 135.0f, 0.0f, 285.0f };

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

    if (std::shared_ptr<Mesh> axesMesh = GetRegisteredMesh("WorldAxes"))
    {
        Actor* axesActor = myWorld.CreateActor("World Axes Actor");
        if (axesActor != nullptr)
        {
            StaticMeshComponent* axesMeshComponent = axesActor->AddComponent<StaticMeshComponent>("World Axes Mesh Component", axesMesh);
            AssignMaterialToAllSlots(axesMeshComponent, CreateMaterialFromFile(materialRoot / "AxesMaterial.mat"));
        }
    }

    if (std::shared_ptr<Mesh> floorMesh = GetRegisteredMesh("Floor"))
    {
        Actor* floorActor = myWorld.CreateActor("Floor Actor");
        if (floorActor != nullptr)
        {
            StaticMeshComponent* floorMeshComponent = floorActor->AddComponent<StaticMeshComponent>("Floor Mesh Component", floorMesh);
            floorActor->SetTranslation(floorPosition);
            floorActor->SetRotation(0.0f, -90.0f, 0.0f);
            floorActor->SetScale({ 1100.0f, 1100.0f, 1100.0f });
            AssignMaterialToAllSlots(floorMeshComponent, CreateMaterialFromFile(materialRoot / "FloorMaterial.mat"));
        }
    }

    if (std::shared_ptr<Mesh> chestMesh = GetRegisteredMesh("SM_Chest"))
    {
        Actor* chestActor = myWorld.CreateActor("SM_Chest Actor");
		StaticMeshComponent* chestMeshComponent = nullptr;
        if (chestActor != nullptr)
        {
            chestMeshComponent = chestActor->AddComponent<StaticMeshComponent>("SM_Chest Mesh Component", chestMesh);
            chestActor->SetTranslation(chestPosition);
            chestActor->SetRotation(-18.0f, 0.0f, 0.0f);
            chestActor->SetScale({ 1.0f, 1.0f, 1.0f });
        }

		AssignMaterialToAllSlots(chestMeshComponent, CreateMaterialFromFile(materialRoot / "ChestMaterial.mat"));
    }

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
                AssignMaterialToAllSlots(myAnimatedMeshComponent, CreateMaterialFromFile(materialRoot / "CharacterMaterial.mat"));

                // TODO Engine future:
                // Replace hardcoded joint mask with data-driven animation mask assets.
                // Masks should be authored externally and resolved to joint indices when loading the skeleton.
                myAnimatedMeshComponent->ConfigurePartialLayerFromJointName("RightShoulder");
                myAnimatedMeshComponent->PlayAnimation("Breathing", true);
            }
        }
    }

    const std::array primitivePlacements = {
        PrimitivePlacement{ "Cube", "", { -310.0f, 100.0f, 120.0f }, { 12.0f, 28.0f, -8.0f }, { 95.0f, 95.0f, 95.0f } },
        PrimitivePlacement{ "Pyramid", "", { 315.0f, 100.0f, 145.0f }, { -8.0f, -34.0f, 12.0f }, { 105.0f, 105.0f, 105.0f } },
        PrimitivePlacement{ "Sphere", "", { -270.0f, 100.0f, 470.0f }, { 0.0f, 0.0f, 0.0f }, { 105.0f, 105.0f, 105.0f } },
        PrimitivePlacement{ "SmoothSphere", "Sphere", { 0.0f, 100.0f, 540.0f }, { 0.0f, 0.0f, 0.0f }, { 115.0f, 115.0f, 115.0f } },
        PrimitivePlacement{ "Torus", "", { 300.0f, 100.0f, 505.0f }, { 24.0f, 42.0f, 0.0f }, { 115.0f, 115.0f, 115.0f } },
    };

    for (const PrimitivePlacement& placement : primitivePlacements)
    {
        const std::string primitiveName = placement.Name;
        std::shared_ptr<Mesh> mesh = GetRegisteredMesh(primitiveName);
        if (mesh == nullptr)
        {
            continue;
        }

        Actor* actor = myWorld.CreateActor(primitiveName + " Actor");
        if (actor != nullptr)
        {
            StaticMeshComponent* meshComponent = actor->AddComponent<StaticMeshComponent>(primitiveName + " Mesh Component", mesh);
            actor->SetTranslation(placement.Position);
            actor->SetRotation(placement.RotationDegrees.x, placement.RotationDegrees.y, placement.RotationDegrees.z);
            actor->SetScale(placement.Scale);
            const std::string materialName = placement.MaterialName[0] != '\0' ? placement.MaterialName : placement.Name;
            AssignMaterialToAllSlots(meshComponent, CreateMaterialFromFile(materialRoot / (materialName + "Material.mat")));
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

void ModelViewer::HandleLightInput()
{
    const bool shiftDown = IsShiftDown(myInputHandler);
    GraphicsEngine& graphicsEngine = GraphicsEngine::Get();

    if (myInputHandler.IsKeyPressed(Keys::F5))
    {
        graphicsEngine.ResetShadowTuning();
    }

    if (myInputHandler.IsKeyPressed(Keys::F6))
    {
        graphicsEngine.AdjustShadowBias(LightType::Directional, -0.00005f);
    }

    if (myInputHandler.IsKeyPressed(Keys::F7))
    {
        graphicsEngine.AdjustShadowBias(LightType::Directional, 0.00005f);
    }

    if (myInputHandler.IsKeyPressed(Keys::F8))
    {
        graphicsEngine.AdjustShadowBias(LightType::Spot, -0.00002f);
    }

    if (myInputHandler.IsKeyPressed(Keys::F9))
    {
        graphicsEngine.AdjustShadowBias(LightType::Spot, 0.00002f);
    }

    if (myInputHandler.IsKeyPressed(Keys::F10))
    {
        graphicsEngine.AdjustShadowBias(LightType::Point, -0.00005f);
    }

    if (myInputHandler.IsKeyPressed(Keys::F11))
    {
        graphicsEngine.AdjustShadowBias(LightType::Point, 0.00005f);
    }

    if (myInputHandler.IsKeyPressed(Keys::P))
    {
        PrintLightTuningValues(myDirectionalLightComponent, myPointLightComponents, mySpotLightComponent);
    }

    if (shiftDown && myCameraActor != nullptr)
    {
        const CommonUtilities::Transform& cameraTransform = myCameraActor->GetTransform();
        const Vector3f cameraPosition = cameraTransform.GetPosition();

        if (IsKeyPressed(myInputHandler, Keys::NUMPAD7, '7') && myDirectionalLightComponent != nullptr)
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

        if (IsKeyPressed(myInputHandler, Keys::NUMPAD8, '8'))
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

        if (IsKeyPressed(myInputHandler, Keys::NUMPAD9, '9') && mySpotLightComponent != nullptr)
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

    if (IsKeyPressed(myInputHandler, Keys::NUMPAD7, '7')
        && myDirectionalLightComponent != nullptr)
    {
        myDirectionalLightComponent->SetEnabled(!myDirectionalLightComponent->IsEnabled());
    }

    if (IsKeyPressed(myInputHandler, Keys::NUMPAD8, '8'))
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

    if (IsKeyPressed(myInputHandler, Keys::NUMPAD9, '9')
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
