#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "GameFramework/Runtime/GameContext.h"
#include "GameFramework/Input/GameInput.h"

#include "MeshLibrary.h"

class Actor;
class Mesh;
class DirectionalLightComponent;
class MaterialInterface;
class PointLightComponent;
class SkeletalMeshComponent;
class SpotLightComponent;
class StaticMeshComponent;

// Temporary authored-scene example, kept in the game project. It creates actors and
// wires dependencies once; it does not own or tick those actors. The eventual JSON
// loader should replace the hardcoded construction with validated scene data and
// component factories.
class ModelViewerScene final
{
public:
	// One-time construction for an empty session world. Calling again does not unload
	// the previous scene and would collide with existing actor names.
	void Initialize(GameContext& context);

private:
	std::shared_ptr<Mesh> GetRegisteredMesh(const std::string& aName) const;
	std::shared_ptr<MaterialInterface> GetMaterial(const std::filesystem::path& aMaterialFile);
	StaticMeshComponent* CreateStaticMeshActor(
		const std::string& anActorName,
		const std::string& aComponentName,
		const std::string& aMeshName,
		const std::filesystem::path& aMaterialFile,
		const CommonUtilities::Vector3<float>& aPosition,
		const CommonUtilities::Vector3<float>& aRotationDegrees,
		const CommonUtilities::Vector3<float>& aScale);
	void LoadScene();
	MeshLibrary myMeshLibrary;
	// Borrowed session references used for construction and wiring. World/Actor own
	// the pointed-to objects. These raw pointers are not cross-scene entity handles.
	GameContext* myContext = nullptr;
	Actor* myCameraActor = nullptr;
	SkeletalMeshComponent* myAnimatedMeshComponent = nullptr;
	DirectionalLightComponent* myDirectionalLightComponent = nullptr;
	std::vector<PointLightComponent*> myPointLightComponents;
	SpotLightComponent* mySpotLightComponent = nullptr;

	std::filesystem::path myContentRoot;
	std::unordered_map<std::string, std::shared_ptr<MaterialInterface>> myMaterialCache;
};
