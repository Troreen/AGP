#pragma once

#include "Drawers/SpriteDrawer.h"
#include "TextWidget.h"

class MeshComponentBase;
class CameraComponent;
class LightComponent;
class ParticleComponent;

struct RenderSnapshot
{
	// Elapsed scene simulation seconds, shared by camera and shadow material draws.
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;
	CameraComponent* ActiveCamera = nullptr;

	std::vector<MeshComponentBase*> AllMeshes;
	std::vector<MeshComponentBase*> OpaqueMeshes;
	std::vector<MeshComponentBase*> BlendedMeshes;
	std::vector<LightComponent*> Lights;
	std::vector<ParticleComponent*> Particles;
	std::vector<std::shared_ptr<TextWidget>> ScreenTextItems;
	std::vector<SpriteDrawCall> ScreenSpriteItems;

	void Clear();
};
