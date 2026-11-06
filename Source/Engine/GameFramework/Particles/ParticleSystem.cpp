#include "GameFramework.pch.h"
#include "ParticleSystem.h"

#include "ParticleEmitter.h"

ParticleSystem::~ParticleSystem()
{
	for (size_t i = 0; i < myEmitters.size(); ++i)
	{
		delete myEmitters[i];
	}
}

void ParticleSystem::Update(float aDeltaTime)
{
	for (size_t i = 0; i < myEmitters.size(); ++i)
	{
		myEmitters[i]->Update(aDeltaTime);
	}
}

const std::vector<ParticleEmitter*>& ParticleSystem::GetEmitters() const
{
	return myEmitters;
}
