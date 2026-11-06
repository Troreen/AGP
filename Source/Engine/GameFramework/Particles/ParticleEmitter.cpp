#include "GameFramework.pch.h"
#include "ParticleEmitter.h"
#include "ServiceLocator.h"

ParticleEmitterSettings::ParticleEmitterSettings()
{
	Color.AddKey(0.0f, CU::Vector4f(1, 0, 0, 0));
	Color.AddKey(0.25f, CU::Vector4f(0, 1, 0, 1));
	Color.AddKey(0.5f, CU::Vector4f(0, 0, 1, 1));
	Color.AddKey(0.75f, CU::Vector4f(1, 1, 0, 1));
	Color.AddKey(1.0f, CU::Vector4f(0, 1, 1, 0));

	Size.AddKey(0.0f, CU::Vector2f(0.0f, 0.0f));
	Size.AddKey(0.25f, CU::Vector2f(10.0f, 10.0f));
	Size.AddKey(0.75f, CU::Vector2f(10.0f, 10.0f));
	Size.AddKey(1.0f, CU::Vector2f(0.0f, 0.0f));

	Angle.AddKey(0.0f, 0.0f);

	Speed.AddKey(0.0f, 0.0f);
}

ParticleEmitterSettings::~ParticleEmitterSettings() = default;

ParticleEmitter::ParticleEmitter() : mySettings(nullptr), myIsPlaying(false)
{
}

ParticleEmitter::~ParticleEmitter()
{
	delete mySettings;
}

void ParticleEmitter::Play()
{
	for (size_t i = 0; i < myParticles.size(); ++i)
	{
		ParticleVertex& particle = myParticles[i];
		InitParticle(particle, i, mySettings->SpawnDelay);
	}

	myIsPlaying = true;
}

void ParticleEmitter::Stop()
{
	myIsPlaying = false;
}

void ParticleEmitter::Update(float aDeltaTime)
{
	if (!myIsPlaying)
	{
		return;
	}

	for (size_t i = 0; i < myParticles.size(); ++i)
	{
		ParticleVertex& particle = myParticles[i];
		particle.Lifetime += aDeltaTime;
		if (particle.Lifetime < 0.0f)
		{
			continue;
		}

		if (particle.Lifetime >= mySettings->Lifetime)
		{
			InitParticle(particle, i);
			continue;
		}

		UpdateParticle(particle, i, aDeltaTime);
	}
}

void ParticleEmitter::InitParticle(ParticleVertex& aParticle, size_t aParticleIndex, float aSpawnDelay)
{
	const float inverseSpawnRate = 1.0f / mySettings->SpawnRate;
	aParticle.Lifetime = 0.0f - (static_cast<float>(aParticleIndex) * inverseSpawnRate) - aSpawnDelay;
	aParticle.Position = { 0, 0, 0, 1 };
	aParticle.Color = mySettings->Color.Get(0);
	aParticle.Size = mySettings->Size.Get(0);
	aParticle.ChannelMask = mySettings->ChannelMask;

	switch (mySettings->SpawnShape)
	{
		case ParticleEmitterSettings::SpawnerShape::Sphere:
			SetSphereShape(aParticle);
			break;

		case ParticleEmitterSettings::SpawnerShape::Cube:
			SetCubeShape(aParticle);
			break;

		case ParticleEmitterSettings::SpawnerShape::Cone:
			SetConeShape(aParticle, CU::Matrix4f::Identity);
			break;
	}
}

void ParticleEmitter::UpdateParticle(ParticleVertex& aParticle, size_t aParticleIndex, float aDeltaTime)
{
	aParticleIndex;

	const float lifePercent = aParticle.Lifetime / mySettings->Lifetime;
	const float gravityAcceleration = -9.81f * mySettings->GravityScale;
	const float totalGainedGravityAcceleration = gravityAcceleration * (aParticle.Lifetime - aDeltaTime);

	aParticle.Velocity.y -= totalGainedGravityAcceleration;
	aParticle.Velocity = aParticle.Velocity.GetNormalized() * mySettings->Speed.Get(lifePercent);
	aParticle.Velocity.y += totalGainedGravityAcceleration + gravityAcceleration * aDeltaTime;

	aParticle.Angle = mySettings->Angle.Get(lifePercent);
	aParticle.Position += aParticle.Velocity.XYZ0() * aDeltaTime;
	aParticle.Size = mySettings->Size.Get(lifePercent);
	aParticle.Color = mySettings->Color.Get(lifePercent);
}

void ParticleEmitter::InitInternal()
{
	const float minSphereRadius = (1 - mySettings->SphereSpawnSettings.radiusThickness) * mySettings->SphereSpawnSettings.radius;
	mySettings->sphereRadiusDist = std::uniform_real_distribution<float>(minSphereRadius, mySettings->SphereSpawnSettings.radius);

	const float cubeHalfX = mySettings->CubeSpawnSettings.size.x * 0.5f;
	const float cubeHalfY = mySettings->CubeSpawnSettings.size.y * 0.5f;
	const float cubeHalfZ = mySettings->CubeSpawnSettings.size.z * 0.5f;

	mySettings->cubeOffsetDistX = std::uniform_real_distribution<float>(-cubeHalfX, cubeHalfX);
	mySettings->cubeOffsetDistY = std::uniform_real_distribution<float>(-cubeHalfY, cubeHalfY);
	mySettings->cubeOffsetDistZ = std::uniform_real_distribution<float>(-cubeHalfZ, cubeHalfZ);

	mySettings->ConeSpawnSettings.angle *= CU::Math::DEGREES_TO_RADIANS<float>;
	mySettings->coneAngleDist = std::uniform_real_distribution<float>(0.0f, mySettings->ConeSpawnSettings.angle);

	const size_t numParticles = static_cast<size_t>(ceilf(mySettings->SpawnRate * mySettings->Lifetime));
	myParticles.resize(numParticles);

	myIsPlaying = mySettings->playOnAwake;
	if (myIsPlaying)
	{
		Play();
	}
}

void ParticleEmitter::SetSphereShape(ParticleVertex& aParticle)
{
	std::mt19937& rngEngine = ServiceLocator::GetInstance().GetRngEngine();
	const float angle = mySettings->twoPiDist(rngEngine);
	const float u = mySettings->sphereUDist(rngEngine);
	const float k = sqrt(1 - (u * u));
	aParticle.Velocity.x = k * cos(angle);
	aParticle.Velocity.y = k * sin(angle);
	aParticle.Velocity.z = u;

	aParticle.Position += aParticle.Velocity.XYZ0() * mySettings->sphereRadiusDist(rngEngine);
}

void ParticleEmitter::SetCubeShape(ParticleVertex& aParticle)
{
	std::mt19937& rngEngine = ServiceLocator::GetInstance().GetRngEngine();
	CU::Vector3f offset(mySettings->cubeOffsetDistX(rngEngine), mySettings->cubeOffsetDistY(rngEngine), mySettings->cubeOffsetDistZ(rngEngine));
	aParticle.Position += offset.XYZ0();
	offset.Normalize();
	aParticle.Velocity = offset;
}

void ParticleEmitter::SetConeShape(ParticleVertex& aParticle, const CU::Matrix4f& aOwnerTransform)
{
	std::mt19937& rngEngine = ServiceLocator::GetInstance().GetRngEngine();
	const float coneAngle = mySettings->coneAngleDist(rngEngine);
	const float circleAngle = mySettings->twoPiDist(rngEngine);

	const float cosConeAngle = cos(coneAngle);
	std::uniform_real_distribution<float> coneZDist(cosConeAngle, 1.0f);
	const float z = coneZDist(rngEngine);
	const float k = sqrt(1 - (z * z));

	CU::Matrix4f rotationTrans = CU::Matrix4f::FromRotation(mySettings->ConeSpawnSettings.rotation.x,
		mySettings->ConeSpawnSettings.rotation.y,
		mySettings->ConeSpawnSettings.rotation.z);

	const float cosCircle = cos(circleAngle);
	const float sinCircle = sin(circleAngle);
	if (!mySettings->local)
	{
		rotationTrans = aOwnerTransform * rotationTrans;
	}
	const CU::Vector3f offset = (CU::Vector4f(cosCircle, sinCircle, 0, 0) * rotationTrans).XYZ();

	aParticle.Velocity = (CU::Vector4f(k * cosCircle, k * sinCircle, z, 0) * rotationTrans).XYZ();
	const float radius = (coneAngle / (mySettings->ConeSpawnSettings.angle)) * mySettings->ConeSpawnSettings.radius;
	aParticle.Position += offset.XYZ0() * radius;
}
