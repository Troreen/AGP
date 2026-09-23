#pragma once
#include "GameFramework/World/Actor.h"

class CameraComponent;
class WorldRenderer;

class World
{
public:
	World() = default;

	~World();
	World(const World&) = delete;
	World& operator=(const World&) = delete;
	Actor* SpawnActor(std::string name);
	Actor* FindActor(const std::string& name) const;


	bool SetActiveCamera(CameraComponent* camera);
	CameraComponent* GetActiveCamera() const;

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
	friend class Actor;
	friend class WorldRenderer;
};
