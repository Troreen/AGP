#pragma once
#include "GameFramework/Components/Component.h"

#include <vector>

class GameContext;
class DirectionalLightComponent;
class PointLightComponent;
class SpotLightComponent;
class SkeletalMeshComponent;

// Game behaviors run on the gameplay owner. Service getters are valid after attachment.
class CameraControlsComponent final : public Component
{
public:
	void BeginPlay() override;
	void LateUpdate(float deltaTime) override;
private:
	float myYaw = 0;
    float myPitch = 0;

};

// Update example: issue playback requests to a sibling SkeletalMeshComponent.
// Attach before the mesh component so requests affect its Update in the same frame.
class AnimationControlsComponent final : public Component
{
public:
	void ResolveReferences(References& context) override;
	void Update(float deltaTime) override;
private:
    ComponentHandle<SkeletalMeshComponent> myMesh;
};

// FixedUpdate example: simulate a simple rotation at the configured constant step.
// This demo assumes an initially unrotated chest and owns all of its rotation.
// R toggles spinning in the fixed input domain, including when a frame has no tick.
class SpinComponent final : public Component
{
public:
	void FixedUpdate(float deltaTime) override;
private:
	float myYaw = 0;
	bool mySpinning = true;
};

// Attached to a scene-controls actor after the camera so LateUpdate uses its final pose.
// Cross-actor behavior example: references are wired during scene creation.
// Handles become empty when a target is destroyed or its scene unloads.
class LightControlsComponent final : public Component
{
public:
	void ResolveReferences(References& context) override;
	void LateUpdate(float deltaTime) override;
    ActorRef Camera;
    ComponentRef<DirectionalLightComponent> Directional;
    ComponentRef<PointLightComponent> Point;
    ComponentRef<SpotLightComponent> Spot;
};
