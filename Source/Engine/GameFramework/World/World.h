#pragma once
#include "GameFramework/World/Actor.h"
#include "GameFramework/Runtime/InputSystem.h"

class CameraComponent;
class WorldRenderer;

class World
{
public:
	explicit World(InputSystem* input = nullptr) : myInput(input)
	{
	}

	~World();
	World(const World&) = delete;
	World& operator=(const World&) = delete;
	Actor* SpawnActor(std::string name);
	Actor* FindActor(const std::string& name) const;

	InputSystem& GetInputSystem() const { return myInput ? *myInput : myEmptyInput; }

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
	InputSystem* myInput;
	mutable InputSystem myEmptyInput;
	CameraComponent* myCamera = nullptr;
	bool myUpdating = false;
	bool myClearing = false;
	friend class Actor;
	friend class WorldRenderer;
};
