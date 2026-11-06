#pragma once

#include <Vector>

class ParticleEmitter;

class ParticleSystem
{
public:
	virtual ~ParticleSystem();
	void Update(float aDeltaTime);

	template<class EmitterType, class EmitterSettingsType>
	void AddEmitter(const EmitterSettingsType& aSettings);

	const std::vector<ParticleEmitter*>& GetEmitters() const;

private:
	std::vector<ParticleEmitter*> myEmitters;
};

template<class EmitterType, class EmitterSettingsType>
inline void ParticleSystem::AddEmitter(const EmitterSettingsType& aSettings)
{
	EmitterType* emitter = new EmitterType();
	emitter->Init(aSettings);
	myEmitters.emplace_back(emitter);
}
