#include "GameFramework/Runtime/Internal/RegistryAccess.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/StaticMeshComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GameFramework/Components/LightComponent.h"

void GameFrameworkInternal::RegisterBuiltInComponents(ComponentRegistry& registry)
{
    registry.Register<SceneComponent>("agp.Scene");
    registry.Register<CameraComponent>("agp.Camera");
    registry.Register<StaticMeshComponent>("agp.StaticMesh");
    registry.Register<SkeletalMeshComponent>("agp.SkeletalMesh");
    registry.Register<DirectionalLightComponent>("agp.DirectionalLight");
    registry.Register<PointLightComponent>("agp.PointLight");
    registry.Register<SpotLightComponent>("agp.SpotLight");
}
