#include "GameFramework.pch.h"
#include "ParticleSystemAsset.h"

#include "AssetRegistry.h"
#include "MaterialAsset.h"
#include "TextureAsset.h"
#include "../Particles/ParticleEmitter.h"
#include "../Helper/JsonHelper.hpp"

#include <Logger/Logger.h>

DECLARE_LOG_CATEGORY_WITH_NAME(PSLog, ParticleSystemAssetLog, Verbose);

DEFINE_LOG_CATEGORY(PSLog);

ParticleSystemAsset::ParticleSystemAsset() = default;

ParticleSystemAsset::ParticleSystemAsset(const std::shared_ptr<ParticleSystem>& aParticleSystem) : myParticleSystem(aParticleSystem)
{
}

ParticleSystemAsset::~ParticleSystemAsset() = default;

const std::shared_ptr<ParticleSystem>& ParticleSystemAsset::GetParticleSystem() const
{
	return myParticleSystem;
}

bool ParticleSystemAsset::Load(const std::filesystem::path& aPath, AssetRegistry& aRegistry)
{
	simdjson::padded_string json = simdjson::padded_string::load(aPath.c_str());
	simdjson::dom::parser parser;
	JsonObject root;
	const simdjson::error_code& error = parser.parse(json).get(root);

	if (error)
	{
		return false;
	}

	std::shared_ptr<ParticleSystem> particleSystem = std::make_shared<ParticleSystem>();

	const JsonResult& emitters = root.at_key("Emitters");
	if (emitters.error() == simdjson::NO_SUCH_FIELD)
	{
		return false;
	}

	for (const JsonElement& emitter : emitters)
	{
		ParticleEmitterSettings settings{};

		TMJson::GetCurveElement_s(emitter, "Color", settings.Color);
		TMJson::GetCurveElement_s(emitter, "Size", settings.Size);
		TMJson::GetCurveElement_s(emitter, "Angle", settings.Angle);
		TMJson::GetCurveElement_s(emitter, "Speed", settings.Speed);

		JsonElement coneSpawnSettings;
		if (TMJson::GetElement_s(emitter, "ConeSpawnSettings", coneSpawnSettings))
		{
			TMJson::GetElement_s(coneSpawnSettings, "Radius", settings.ConeSpawnSettings.radius);
			TMJson::GetElement_s(coneSpawnSettings, "Angle", settings.ConeSpawnSettings.angle);
			TMJson::GetElement_s(coneSpawnSettings, "Rotation", settings.ConeSpawnSettings.rotation);
		}

		JsonElement cubeSpawnSettings;
		if (TMJson::GetElement_s(emitter, "CubeSpawnSettings", cubeSpawnSettings))
		{
			TMJson::GetElement_s(cubeSpawnSettings, "Size", settings.CubeSpawnSettings.size);
		}

		JsonElement sphereSpawnSettings;
		if (TMJson::GetElement_s(emitter, "SphereSpawnSettings", sphereSpawnSettings))
		{
			TMJson::GetElement_s(sphereSpawnSettings, "Radius", settings.SphereSpawnSettings.radius);
			TMJson::GetElement_s(sphereSpawnSettings, "RadiusThickness", settings.SphereSpawnSettings.radiusThickness);
		}

		TMJson::GetElement_s(emitter, "ChannelMask", settings.ChannelMask);
		TMJson::GetElement_s(emitter, "SpawnRate", settings.SpawnRate);
		TMJson::GetElement_s(emitter, "Lifetime", settings.Lifetime);
		TMJson::GetElement_s(emitter, "GravityScale", settings.GravityScale);
		TMJson::GetElement_s(emitter, "SpawnDelay", settings.SpawnDelay);
		TMJson::GetElement_s(emitter, "BurstSpawnDelay", settings.BurstSpawnDelay);

		std::string_view spawnShapeString;
		TMJson::GetElement_s(emitter, "SpawnShape", spawnShapeString);
		if (spawnShapeString == "Sphere")
		{
			settings.SpawnShape = ParticleEmitterSettings::SpawnerShape::Sphere;
		}
		else if (spawnShapeString == "Cube")
		{
			settings.SpawnShape = ParticleEmitterSettings::SpawnerShape::Cube;
		}
		else if (spawnShapeString == "Cone")
		{
			settings.SpawnShape = ParticleEmitterSettings::SpawnerShape::Cone;
		}

		TMJson::GetElement_s(emitter, "Looping", settings.looping);
		TMJson::GetElement_s(emitter, "Local", settings.local);
		TMJson::GetElement_s(emitter, "PlayOnAwake", settings.playOnAwake);

		std::string_view materialName;
		TMJson::GetElement_s(emitter, "Material", materialName);
		settings.Material = aRegistry.GetAsset<MaterialAsset>(materialName);

		JsonElement textures;
		if (TMJson::GetElement_s(emitter, "Textures", textures))
		{
			for (const JsonElement& texture : textures)
			{
				unsigned slot;
				std::string_view name;
				if (TMJson::GetElement_s(texture, "Slot", slot) && TMJson::GetElement_s(texture, "Name", name))
				{
					settings.Textures[slot] = aRegistry.GetAsset<TextureAsset>(name);
				}
			}
		}

		particleSystem->AddEmitter<ParticleEmitter>(settings);
	}

	myParticleSystem = std::move(particleSystem);

	return true;
}
