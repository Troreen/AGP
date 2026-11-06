#pragma once

#include <CommonUtilities/CommonUtilities.h>
#include <CommonUtilities/Curve.hpp>
#include <GraphicsEngine/Materials/Material.h>
#include <GraphicsEngine/Objects/Buffer.h>
#include <GraphicsEngine/Objects/ParticleVertex.h>
#include <random>
#include <vector>

class MaterialAsset;
class TextureAsset;

struct ParticleEmitterSettings
{
	enum class SpawnerShape : uint8_t
	{
		Sphere,
		Cube,
		Cone
	};

	struct SphereSpawnSettings
	{
		float radius = 1;
		float radiusThickness = 1;
	};

	struct CubeSpawnSettings
	{
		CU::Vector3f size = { 1.0f, 1.0f, 1.0f };
	};

	struct ConeSpawnSettings
	{
		float radius = 1.0f;
		float angle = 30.0f;
		CU::Vector3f rotation = { 0, 0, 0 };
	};

	CU::Curve<CU::Vector4f> Color;
	CU::Curve<CU::Vector2f> Size;
	CU::Curve<float> Angle;
	CU::Curve<float> Speed;

	ConeSpawnSettings ConeSpawnSettings;
	CubeSpawnSettings CubeSpawnSettings;
	SphereSpawnSettings SphereSpawnSettings;

	CU::Vector4f ChannelMask = { 1, 0, 0, 0 };

	mutable std::uniform_real_distribution<float> twoPiDist = std::uniform_real_distribution<float>(0.0f, static_cast<float>(2.0 * std::numbers::pi));
	mutable std::uniform_real_distribution<float> sphereUDist = std::uniform_real_distribution<float>(-1.0f, 1.0f);

	mutable std::uniform_real_distribution<float> sphereRadiusDist;

	mutable std::uniform_real_distribution<float> cubeOffsetDistX;
	mutable std::uniform_real_distribution<float> cubeOffsetDistY;
	mutable std::uniform_real_distribution<float> cubeOffsetDistZ;

	mutable std::uniform_real_distribution<float> coneAngleDist;

	float SpawnRate = 1.0f;
	float Lifetime = 5.0f;
	float GravityScale = 0.0f;
	float SpawnDelay = 1.0f;
	float BurstSpawnDelay = 0.0f;

	SpawnerShape SpawnShape = SpawnerShape::Sphere;

	bool looping = true;
	bool local = true;
	bool playOnAwake = true;

	std::shared_ptr<MaterialAsset> Material;

	std::unordered_map<unsigned, std::shared_ptr<TextureAsset>> Textures;

	ParticleEmitterSettings();
	virtual ~ParticleEmitterSettings();
};

class ParticleEmitter
{
	friend class GraphicsEngine;

public:
	ParticleEmitter();
	virtual ~ParticleEmitter();

	template<class EmitterSettingsType>
	requires std::is_base_of_v<ParticleEmitterSettings, EmitterSettingsType>
	void Init(const EmitterSettingsType& aSettings);

	void Play();
	void Stop();
	void Update(float aDeltaTime);

	bool IsPlaying() const { return myIsPlaying; }

	template<class EmitterSettingsType>
	requires std::is_base_of_v<ParticleEmitterSettings, EmitterSettingsType>
	EmitterSettingsType* GetEmitterSettings() const;

protected:
	virtual void InitParticle(ParticleVertex& aParticle, size_t aParticleIndex, float aSpawnDelay = 0.0f);
	virtual void UpdateParticle(ParticleVertex& aParticle, size_t aParticleIndex, float aDeltaTime);
	virtual void InitInternal();

	std::vector<ParticleVertex> myParticles;

private:
	void SetSphereShape(ParticleVertex& aParticle);
	void SetCubeShape(ParticleVertex& aParticle);
	void SetConeShape(ParticleVertex& aParticle, const CU::Matrix4f& aOwnerTransform);

	ParticleEmitterSettings* mySettings;

	mutable Buffer myVertexBuffer;

	bool myIsPlaying;
};

template<class EmitterSettingsType>
requires std::is_base_of_v<ParticleEmitterSettings, EmitterSettingsType>
inline void ParticleEmitter::Init(const EmitterSettingsType& aSettings)
{
	mySettings = new EmitterSettingsType();
	*mySettings = aSettings;
	InitInternal();
}

template<class EmitterSettingsType>
requires std::is_base_of_v<ParticleEmitterSettings, EmitterSettingsType>
inline EmitterSettingsType* ParticleEmitter::GetEmitterSettings() const
{
	return dynamic_cast<EmitterSettingsType*>(mySettings);
}
