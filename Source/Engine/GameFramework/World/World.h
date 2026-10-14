#pragma once
#include "GameFramework/World/Actor.h"
#include "Vector2.hpp"

#include <GraphicsEngine/RenderSnapshot.h>

class CameraComponent;

class World
{
	friend class Actor;

public:
	World() = default;

	~World();
	World(const World&) = delete;
	World& operator=(const World&) = delete;
	Actor* SpawnActor(std::string name);
	Actor* FindActor(const std::string& name) const;

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

	std::vector<std::unique_ptr<Actor>> myActors;
	CameraComponent* myCamera = nullptr;
	bool myUpdating = false;
	bool myClearing = false;

	Actor* myDebugActor;

	RenderSnapshot myWorldSnapshot;
};
