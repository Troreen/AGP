#pragma once

#include "MeshComponentBase.h"

// A mesh without skeletal skinning. Static describes the geometry type, not an
// immovable actor: gameplay can still translate, rotate and scale its owner.
class StaticMeshComponent final : public MeshComponentBase
{
public:
	using MeshComponentBase::MeshComponentBase;
};
