#pragma once

#include "Drawers/SpriteDrawer.h"
#include "TextWidget.h"

#include <GameFramework/Components/MeshComponentBase.h>
#include <GameFramework/Components/CameraComponent.h>
#include <GameFramework/Components/LightComponent.h>

struct RenderSnapshot
{
	CameraComponent* ActiveCamera = nullptr;

	std::vector<MeshComponentBase*> Meshes;
	std::vector<LightComponent*> Lights;
	std::vector<std::shared_ptr<TextWidget>> ScreenTextItems;
	std::vector<SpriteDrawCall> ScreenSpriteItems;

	void Clear();
};
