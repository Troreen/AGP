#pragma once

#include "SceneComponent.h"

class ParticleSystemAsset;

class ParticleComponent : public SceneComponent
{
public:
	void Update(float aDeltaTime) override;

	void SetParticleSystem(const std::shared_ptr<ParticleSystemAsset>& aSystem);
	const std::shared_ptr<ParticleSystemAsset>& GetParticleSystem() const;

private:
	std::shared_ptr<ParticleSystemAsset> myParticleSystem;
};
