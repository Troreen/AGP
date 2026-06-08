#pragma once

#include "Component.h"

#include <memory>

class Mesh;

class MeshComponent final : public Component
{
public:
	MeshComponent() = default;
	explicit MeshComponent(std::shared_ptr<Mesh> aMesh);

	void SetMesh(std::shared_ptr<Mesh> aMesh);
	std::shared_ptr<Mesh> GetMesh() const;
	bool HasMesh() const;

	void SetVisible(bool aVisible);
	bool IsVisible() const;

private:
	std::shared_ptr<Mesh> myMesh;
};
