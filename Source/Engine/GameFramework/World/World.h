#pragma once
#include "GameFramework/World/Actor.h"
#include "Vector2.hpp"
#include <cstdint>

#include <GraphicsEngine/RenderSnapshot.h>

class CameraComponent;
class ColliderComponent;
class CollisionSystem;

class World
{
	friend class Actor;

public:
	World();

	~World();
	World(const World&) = delete;
	World& operator=(const World&) = delete;
	Actor* SpawnActor(std::string_view name);
	Actor* FindActor(std::string_view name) const;
	ColliderComponent* FindCollider(std::uint64_t id) const;

	bool SetActiveCamera(CameraComponent* camera);
	CameraComponent* GetActiveCamera() const;
	bool SetCameraResolution(const CommonUtilities::Vector2u& aResolution);

	// The application drives these; normal gameplay only spawns, finds and destroys.
	void BeginPlay();
	void Update(float deltaTime);
	void Clear();

	// TODO: Would it be better to add MeshComponents and LightComponents to lists
	//    when they are created/activated and removed when they are destroyed/deactivated?
	RenderSnapshot& GenerateRenderSnapshot();

private:
	std::vector<Component*> CollectFrame();
	void RemoveDestroyed();
	void RegisterCollider(Component& component);
	std::unique_ptr<CollisionSystem> myCollisionSystem;

	std::vector<std::unique_ptr<Actor>> myActors;
	std::unordered_map<std::string, size_t> myActorNameCount;
	CameraComponent* myCamera = nullptr;
	bool myUpdating = false;
	bool myClearing = false;
	double mySimulationTime = 0.0;

	Actor* myDebugActor;

	RenderSnapshot myWorldSnapshot;
};
