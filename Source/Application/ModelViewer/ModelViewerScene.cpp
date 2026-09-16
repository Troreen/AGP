#include "ModelViewerScene.h"
#include "ModelViewerComponents.h"
#include "GraphicsEngine/GraphicsEngine.h"
#include "GraphicsEngine/Objects/Mesh.h"

#include <cstddef>
#include <memory>
#include <string>

#include "Application.h"
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/World/Actor.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GameFramework/Components/StaticMeshComponent.h"
#include "GraphicsEngine/Materials/Material.h"

namespace
{
	using Vector3f = CommonUtilities::Vector3f;
	using Vector4f = CommonUtilities::Vector4f;
	// The build copies game materials and shader snippets into Assets/Shaders.
	// Material texture paths are relative to that runtime directory (../Textures),
	// not to this source file or the process working directory.
	std::filesystem::path GetMaterialRoot(const std::filesystem::path& aContentRoot)
	{
		return aContentRoot / "Shaders";
	}

	// Example authoring shortcut: apply one material to every submesh slot.
	// A data-driven scene can instead author different materials for individual slots.
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

	PointLightComponent* CreatePointLight(World& aWorld, const char* aName, const Vector3f& aPosition, const Vector3f& aColor,
	                                      float anIntensity, float aRadius)
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

// Load the mesh/animation catalog first, instantiate the scene second, then select
// the camera. This all runs during IGame::Initialize, before concurrent rendering.
// Future asset services should preserve that safe handoff when adding async loading.
void ModelViewerScene::Initialize(GameContext& context)
{
	myContext = &context;
	myContentRoot = context.GetContentRoot();
	myMeshLibrary.Initialize(myContentRoot);
	LoadScene();
	context.SetActiveCamera(myCameraActor);
}

// This function is an executable example of scene authoring, not the intended
// long-term scene format. Names, transforms, asset references and behavior settings
// are the data a future JSON import should supply. The C++ component implementations
// remain responsible for behavior after loading; JSON does not need to know threading.
void ModelViewerScene::LoadScene()
{
	myAnimatedMeshComponent = nullptr;
	myDirectionalLightComponent = nullptr;
	myPointLightComponents.clear();
	mySpotLightComponent = nullptr;
	const std::filesystem::path materialRoot = GetMaterialRoot(myContentRoot);
	const Vector3f sceneFocus = {25.0f, 0.0f, 260.0f};
	const Vector3f floorPosition = {0.0f, 0.0f, 260.0f};
	const Vector3f characterPosition = {0.0f, 0.0f, 250.0f};
	const Vector3f chestPosition = {135.0f, 0.0f, 285.0f};
	const Vector3f chestAlphaPosition = {-200.0f, 0.0f, -100.0f};
	const Vector3f colorCheckerPosition = {-145.0f, 40.0f, 365.0f};

	// --- Camera actor ---
	// Engine CameraComponent supplies projection data; game CameraControlsComponent
	// supplies movement. This demonstrates composing reusable features with game rules.
	// Create the camera before scene controls so its LateUpdate runs first.
	{
		myCameraActor = myContext->GetWorld().CreateActor("Camera Actor");
		if (myCameraActor != nullptr)
		{
			myCameraActor->AddComponent<CameraComponent>("Camera", 90.0f, 1.0f, 50000.0f, myContext->GetClientSize());

			myCameraActor->SetTranslation({0.0f, 260.0f, -950.0f});
			myCameraActor->LookAt(sceneFocus);
			myCameraActor->AddComponent<CameraControlsComponent>("Camera Controls", *myContext);
		}
	}

	// --- Lighting ---
	// Create ordinary actors with engine light components. The later LightControls
	// component receives references to these lights; the renderer discovers them from
	// the world, so game code never registers lights with render workers.
	{
		Actor* directionalLightActor = myContext->GetWorld().CreateActor("Directional Light Actor");
		if (directionalLightActor != nullptr)
		{
			directionalLightActor->SetTranslation({-450.0f, 650.0f, -350.0f});
			directionalLightActor->LookAt(sceneFocus);
			myDirectionalLightComponent = directionalLightActor->AddComponent<DirectionalLightComponent>("Directional Light");
			if (myDirectionalLightComponent != nullptr)
			{
				myDirectionalLightComponent->SetColor({1.0f, 0.96f, 0.9f});
				myDirectionalLightComponent->SetIntensity(5.0f);
			}
		}

		myPointLightComponents.push_back(
		    CreatePointLight(myContext->GetWorld(), "Warm Character Point", {-90.0f, 180.0f, 150.0f}, {1.0f, 0.42f, 0.22f}, 20.0f, 760.0f));

		Actor* spotLightActor = myContext->GetWorld().CreateActor("Spot Light Actor");
		if (spotLightActor != nullptr)
		{
			spotLightActor->SetTranslation({430.0f, 430.0f, -210.0f});
			spotLightActor->LookAt(sceneFocus);
			mySpotLightComponent = spotLightActor->AddComponent<SpotLightComponent>("Spot Light");
			if (mySpotLightComponent != nullptr)
			{
				mySpotLightComponent->SetColor({0.55f, 0.7f, 1.0f});
				mySpotLightComponent->SetIntensity(30.0f);
				mySpotLightComponent->SetRadius(1200.0f);
				mySpotLightComponent->SetConeAnglesDegrees(18.0f, 34.0f);
			}
		}
	}

	// --- Mesh actors and game behavior ---
	// Several actors can share a mesh/material while retaining independent transforms.
	// The opaque chest gains a SpinComponent; the floor has no behavior to tick.
	CreateStaticMeshActor("Floor Actor", "Floor Mesh Component", "Floor", materialRoot / "FloorMaterial.mat", floorPosition,
	                      {0.0f, -90.0f, 0.0f}, {1100.0f, 1100.0f, 1100.0f});

	StaticMeshComponent* chest = CreateStaticMeshActor("SM_Chest Actor", "SM_Chest Mesh Component", "SM_Chest", materialRoot / "ChestMaterial.mat", chestPosition,
	                      {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

	if (chest) chest->GetOwner()->AddComponent<SpinComponent>("Spin", *myContext); //TODO: context shouldnt be passed around like this. figure out a better way to access input handling

	// Create a material instance when one actor needs different authored parameters.
	// Configure it during initialization; changing shared material contents during live
	// rendering is not supported by the current snapshot ownership contract.
	StaticMeshComponent* alphaChestMeshComponent =
	    CreateStaticMeshActor("SM_Chest Alpha Actor", "SM_Chest Alpha Mesh Component", "SM_Chest", materialRoot / "ChestMaterial_Alpha.mat",
	                          chestAlphaPosition, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

	if (alphaChestMeshComponent != nullptr)
	{
		if (const std::shared_ptr<MaterialInterface> alphaBaseMaterial = GetMaterial(materialRoot / "ChestMaterial_Alpha.mat"))
		{
			if (const std::shared_ptr<MaterialInstance> alphaChestMaterial =
			        MaterialInstance::Create("ChestMaterial_Alpha_Instance", alphaBaseMaterial))
			{
				alphaChestMaterial->SetValue("MB_Tint", Vector4f(1.0f, 1.0f, 1.0f, 0.35f));
				AssignMaterialToAllSlots(alphaChestMeshComponent, alphaChestMaterial);
			}
		}
	}

	CreateStaticMeshActor("SM_Color_Checker Actor", "SM_Color_Checker Mesh Component", "SM_Color_Checker",
	                      materialRoot / "ColorCheckerMaterial.mat", colorCheckerPosition, {0.0f, -90.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

	// --- Animated actor ---
	// The mesh library supplies named animation assets. Game controls choose playback;
	// SkeletalMeshComponent owns the per-actor playback state and advances the pose.
	if (std::shared_ptr<Mesh> characterMesh = GetRegisteredMesh("SK_C_TGA_Bro"))
	{
		Actor* characterActor = myContext->GetWorld().CreateActor("TGA Bro Actor");
		if (characterActor != nullptr)
		{
			// Controls precede the mesh so animation requests apply in this frame.
			characterActor->AddComponent<AnimationControlsComponent>("Animation Controls", *myContext);
			myAnimatedMeshComponent = characterActor->AddComponent<SkeletalMeshComponent>("TGA Bro Mesh Component", characterMesh);
			characterActor->SetTranslation(characterPosition);
			characterActor->SetRotation(180.0f, 0.0f, 0.0f);
			characterActor->SetScale({1.0f, 1.0f, 1.0f});

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
	// --- Cross-actor controls ---
	// An actor need not render anything. This one hosts behavior that operates on the
	// lights and camera. It is inserted last so its LateUpdate sees the camera after
	// CameraControls has moved it. This is explicit insertion order, not a dependency graph.
	Actor* sceneControls = myContext->GetWorld().CreateActor("Scene Controls");
	sceneControls->AddComponent<LightControlsComponent>("Light Controls", *myContext, myCameraActor,
		myDirectionalLightComponent, myPointLightComponents, mySpotLightComponent);

}

// Initialization-only material loading/cache for this example. The description
// loader resolves paths relative to the .mat file; graphics creates the render asset.
// A reusable content service should eventually own this mechanism while the game
// continues to choose which material assets its scenes reference.
std::shared_ptr<MaterialInterface> ModelViewerScene::GetMaterial(const std::filesystem::path& aMaterialFile)
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

// Small scene-authoring helper: resolve shared assets, create a world-owned actor,
// attach its render component, then apply authored transform/material values.
// A future scene factory can perform these same steps from SceneDescription data.
StaticMeshComponent* ModelViewerScene::CreateStaticMeshActor(const std::string& anActorName, const std::string& aComponentName,
														const std::string& aMeshName, const std::filesystem::path& aMaterialFile,
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

	Actor* actor = myContext->GetWorld().CreateActor(anActorName);
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

std::shared_ptr<Mesh> ModelViewerScene::GetRegisteredMesh(const std::string& aName) const
{
	return myMeshLibrary.GetMesh(aName);
}

