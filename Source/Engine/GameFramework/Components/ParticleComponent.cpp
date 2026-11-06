#include "GameFramework.pch.h"
#include "ParticleComponent.h"
#include "../AssetHandling/ParticleSystemAsset.h"

void ParticleComponent::Update(float aDeltaTime)
{
	myParticleSystem->GetParticleSystem()->Update(aDeltaTime);
}

void ParticleComponent::SetParticleSystem(const std::shared_ptr<ParticleSystemAsset>& aSystem)
{
	myParticleSystem = aSystem;
}

const std::shared_ptr<ParticleSystemAsset>& ParticleComponent::GetParticleSystem() const
{
	return myParticleSystem;
}
