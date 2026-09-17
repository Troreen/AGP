#include "GameFramework/Rendering/WorldRenderer.h"
#include "GameFramework/World/World.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/LightComponent.h"
#include "GameFramework/Components/MeshComponentBase.h"
#include <chrono>

void WorldRenderer::Build(const World& world, GraphicsEngine& graphics, GraphicsEngine::RenderSceneSnapshot& snapshot)
{
	const auto start = std::chrono::steady_clock::now();
	snapshot.Clear();
	auto* camera = world.GetActiveCamera();
	if (!camera || !camera->HasBegunPlay() || !camera->IsEnabled() || !camera->GetOwner()->IsActive())
	{
		return;
	}
	camera->SyncCameraToOwner();
	snapshot.Camera = camera->myCamera;
	snapshot.HasCamera = true;
	for (const auto& actor : world.myActors)
	{
		if (!actor->IsActive())
		{
			continue;
		}
		for (const auto& component : actor->myComponents)
		{
			if (!component->HasBegunPlay() || !component->IsEnabled())
			{
				continue;
			}
			if (const auto* light = dynamic_cast<LightComponent*>(component.get()))
			{
				GraphicsEngine::LightSnapshot item;
				item.Type = static_cast<RenderLightType>(light->GetLightType());
				item.Color = light->GetColor();
				item.Intensity = light->GetIntensity();
				item.Position = light->GetWorldPosition();
				item.Direction = light->GetWorldDirection();
				item.InnerCone = light->GetInnerCone();
				item.OuterCone = light->GetOuterCone();
				item.Radius = light->GetRadius();
				snapshot.RelevantLights.push_back(item);
			}
			if (const auto* mesh = dynamic_cast<MeshComponentBase*>(component.get()); mesh && mesh->IsVisible() && mesh->myMesh)
			{
				GraphicsEngine::RenderItemSnapshot item;
				item.Mesh = mesh->myMesh;
				item.Materials = mesh->myMaterials;
				item.World = mesh->GetWorldMatrix();
				item.HasSkinning = mesh->HasSkinning();
				if (const auto* joints = mesh->GetJointTransforms())
				{
					item.JointTransforms = *joints;
				}
				snapshot.ShadowCasters.push_back(std::move(item));
			}
		}
	}
	graphics.FinalizeRenderSnapshot(snapshot);
	snapshot.Stats.SnapshotMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}
