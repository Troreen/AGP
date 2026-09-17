#include "GameFramework/Scenes/ComponentRegistry.h"
#include "GameFramework/Components/CameraComponent.h"
#include "GameFramework/Components/StaticMeshComponent.h"
#include "GameFramework/Components/SkeletalMeshComponent.h"
#include "GameFramework/Components/LightComponent.h"
#include "GraphicsEngine/Objects/Mesh.h"
#include <cmath>

namespace
{
	bool ReadMesh(MeshComponentBase& component, const SceneReader& fields)
	{
		const auto meshId = fields.RequiredAsset("mesh");
		const auto meshBinding = fields.GetAssets().FindMesh(meshId);
		component.SetMesh(meshBinding);
		component.SetVisible(fields.OptionalBool("visible", component.IsVisible()));
		const auto materials = fields.OptionalAssets("materials");
		if (!meshBinding)
		{
			fields.Error("mesh", "Mesh asset is unavailable: " + meshId.Value);
			return false;
		}
		if (materials.size() != component.GetMaterialCount())
		{
			fields.Error("materials", "Material list must exactly match the mesh material slots");
			return false;
		}
		std::vector<MaterialAsset> bindings;
		bool valid = true;
		for (size_t index = 0; index < materials.size(); ++index)
		{
			auto material = fields.GetAssets().FindMaterial(materials[index]);
			if (!material)
			{
				fields.Error("materials[" + std::to_string(index) + "]", "Material asset is unavailable: " + materials[index].Value);
				valid = false;
			}
			bindings.push_back(std::move(material));
		}
		if (!valid)
		{
			return false;
		}
		component.SetMesh(meshBinding);
		for (unsigned index = 0; index < bindings.size(); ++index)
		{
			component.SetMaterial(index, bindings[index]);
		}
		return true;
	}

	void ReadLight(LightComponent& component, const SceneReader& fields)
	{
		const auto color = fields.OptionalVector3("color", component.GetColor());
		const auto intensity = fields.OptionalFloat("intensity", component.GetIntensity());
		if (color.x < 0 || color.y < 0 || color.z < 0)
		{
			fields.Error("color", "Light color channels must be nonnegative");
		}
		else
		{
			component.SetColor(color);
		}
		if (intensity < 0)
		{
			fields.Error("intensity", "Light intensity must be nonnegative");
		}
		else
		{
			component.SetIntensity(intensity);
		}
		if (component.GetLightType() != LightType::Directional)
		{
			const auto radius = fields.OptionalFloat("radius", component.GetRadius());
			if (radius < 1)
			{
				fields.Error("radius", "Light radius must be at least one engine unit");
			}
			else
			{
				component.SetRadius(radius);
			}
		}
		if (component.GetLightType() == LightType::Spot)
		{
			const auto inner = fields.OptionalFloat("innerCone", CommonUtilities::Maths::RadiansToDegrees(component.GetInnerCone()));
			const auto outer = fields.OptionalFloat("outerCone", CommonUtilities::Maths::RadiansToDegrees(component.GetOuterCone()));
			if (inner < 0 || outer < inner || outer > 89)
			{
				fields.Error("outerCone", "Spot cone angles must satisfy 0 <= inner <= outer <= 89 degrees");
			}
			else
			{
				component.SetConeAnglesDegrees(inner, outer);
			}
		}
	}
}

void ComponentRegistry::RegisterBuiltIns()
{
	Register<SceneComponent>("agp.Scene");
	Register<CameraComponent>("agp.Camera", [](CameraComponent& component, const SceneReader& fields)
	{
		const auto fov = fields.OptionalFloat("fov", CameraComponent::DefaultFieldOfView);
		const auto nearPlane = fields.OptionalFloat("near", CameraComponent::DefaultNearPlane);
		const auto farPlane = fields.OptionalFloat("far", CameraComponent::DefaultFarPlane);
		if (fov <= 0 || fov >= 180)
		{
			fields.Error("fov", "Field of view must be strictly between 0 and 180 degrees");
		}
		if (nearPlane <= 0)
		{
			fields.Error("near", "Near plane must be positive");
		}
		if (farPlane <= nearPlane)
		{
			fields.Error("far", "Far plane must be greater than near plane");
		}
		const auto size = fields.GetClientSize();
		if (!size.x || !size.y)
		{
			fields.Error("resolution", "Camera resolution must be nonzero");
		}
		if (fov > 0 && fov < 180 && nearPlane > 0 && farPlane > nearPlane && size.x && size.y)
		{
			if (!component.SetPerspective(fov, nearPlane, farPlane, size))
			{
				fields.Error("projection", "Invalid camera projection");
			}
		}
	});
	Register<StaticMeshComponent>("agp.StaticMesh", [](StaticMeshComponent& component, const SceneReader& fields)
	{
		ReadMesh(component, fields);
	});
	Register<SkeletalMeshComponent>("agp.SkeletalMesh", [](SkeletalMeshComponent& component, const SceneReader& fields)
	{
		const bool meshReady = ReadMesh(component, fields);
		const auto animation = fields.OptionalString("animation");
		const auto loop = fields.OptionalBool("loop", true);
		const auto partialRoot = fields.OptionalString("partialRoot");
		if (!meshReady)
		{
			return;
		}
		if (fields.Has("partialRoot") && (partialRoot.empty() || !component.ConfigurePartialLayerFromJointName(partialRoot)))
		{
			fields.Error("partialRoot", "Partial animation root joint does not exist");
		}
		if (fields.Has("animation") && !component.PlayAnimation(animation, loop))
		{
			fields.Error("animation", "Animation is unavailable or invalid");
		}
	});
	Register<DirectionalLightComponent>("agp.DirectionalLight", [](DirectionalLightComponent& component, const SceneReader& fields)
	{
		ReadLight(component, fields);
	});
	Register<PointLightComponent>("agp.PointLight", [](PointLightComponent& component, const SceneReader& fields)
	{
		ReadLight(component, fields);
	});
	Register<SpotLightComponent>("agp.SpotLight", [](SpotLightComponent& component, const SceneReader& fields)
	{
		ReadLight(component, fields);
	});
}
