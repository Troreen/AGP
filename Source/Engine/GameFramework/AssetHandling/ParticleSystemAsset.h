#pragma once

#include "Asset.h"
#include "../Particles/ParticleSystem.h"

class ParticleSystemAsset : public Asset
{
public:
	ParticleSystemAsset();
	explicit ParticleSystemAsset(const std::shared_ptr<ParticleSystem>& aParticleSystem);
	~ParticleSystemAsset() override;

	const std::shared_ptr<ParticleSystem>& GetParticleSystem() const;

protected:
	bool Load(const std::filesystem::path& aPath, AssetRegistry& aRegistry) override;

private:
	std::shared_ptr<ParticleSystem> myParticleSystem;
};
