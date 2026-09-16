#pragma once
#include "GameFramework/Components/Component.h"
#include "FreeFlyCameraController.h"
#include <vector>

class GameContext;
class DirectionalLightComponent;
class PointLightComponent;
class SpotLightComponent;

// These components belong to this game, not GameFramework. Constructors receive
// GameContext explicitly instead of discovering global input or engine threads.
// Context references are borrowed for the session; access them only during hooks.
// AddComponent assigns the owner after construction, so owner-dependent work waits
// until attachment is complete. See ModelViewerScene.cpp for the wiring examples.
// Game behaviors: the world calls these automatically on enabled, active actors.
// LateUpdate example: convert game input into camera movement after ordinary
// updates. The reusable camera math helper does not choose this game's key bindings.
class CameraControlsComponent final : public Component
{
public:
	explicit CameraControlsComponent(GameContext& context) : myContext(context) {}
	void LateUpdate(float deltaTime) override;
private:
	GameContext& myContext;
	FreeFlyCameraController myController;
	bool myInitialized = false;
};

// Update example: issue playback requests to a sibling SkeletalMeshComponent.
// Attach before the mesh component so requests affect its Update in the same frame.
class AnimationControlsComponent final : public Component
{
public:
	explicit AnimationControlsComponent(GameContext& context) : myContext(context) {}
	void Update(float deltaTime) override;
private:
	GameContext& myContext;
};

// FixedUpdate example: simulate a simple rotation at the configured constant step.
// This demo assumes an initially unrotated chest and owns all of its rotation.
// R toggles spinning in the fixed input domain, including when a frame has no tick.
class SpinComponent final : public Component
{
public:
	explicit SpinComponent(GameContext& context) : myContext(context) {}
	void FixedUpdate(float deltaTime) override;
private:
	GameContext& myContext;
	float myYaw = 0;
	bool mySpinning = true;
};

// Attached to a scene-controls actor after the camera so LateUpdate uses its final pose.
// Cross-actor behavior example: references are wired during scene creation.
// These are non-owning raw pointers, valid for this session; scene unloading will
// require a proper entity-reference/lifecycle system before such links can persist.
class LightControlsComponent final : public Component
{
public:
	LightControlsComponent(GameContext& context, Actor* camera, DirectionalLightComponent* directional,
		std::vector<PointLightComponent*> points, SpotLightComponent* spot);
	void LateUpdate(float deltaTime) override;
private:
	GameContext& myContext;
	Actor* myCameraActor;
	DirectionalLightComponent* myDirectionalLightComponent;
	std::vector<PointLightComponent*> myPointLightComponents;
	SpotLightComponent* mySpotLightComponent;
};
