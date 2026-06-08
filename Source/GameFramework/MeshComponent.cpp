#include "MeshComponent.h"

#include <utility>

MeshComponent::MeshComponent(std::shared_ptr<Mesh> aMesh)
	: myMesh(std::move(aMesh))
{
}

void MeshComponent::SetMesh(std::shared_ptr<Mesh> aMesh)
{
	myMesh = std::move(aMesh);
}

std::shared_ptr<Mesh> MeshComponent::GetMesh() const
{
	return myMesh;
}

bool MeshComponent::HasMesh() const
{
	return myMesh != nullptr;
}

void MeshComponent::SetVisible(bool aVisible)
{
	SetEnabled(aVisible);
}

bool MeshComponent::IsVisible() const
{
	return IsEnabled();
}
