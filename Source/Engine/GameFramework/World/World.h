#pragma once
#include "GameFramework/World/Actor.h"
#include "Vector2.hpp"

class CameraComponent;
class WorldRenderer;

class World
{
	friend class Actor;
	friend class WorldRenderer;

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

private:
	std::vector<Component*> CollectFrame();
	void RemoveDestroyed();
	std::vector<std::unique_ptr<Actor>> myActors;
	CameraComponent* myCamera = nullptr;
	bool myUpdating = false;
	bool myClearing = false;
};
