#pragma once
#include "GameFramework/World/Component.h"
#include "GameFramework/World/Transform.h"

// A camera, mesh or light can have a local offset relative to its Actor.
class SceneComponent : public Component
{
public:
	Transform& GetTransform()
	{
		return myTransform;
	}

	const Transform& GetTransform() const
	{
		return myTransform;
	}

	CommonUtilities::Matrix4f GetWorldMatrix() const;
	CommonUtilities::Vector3f GetWorldPosition() const;
	CommonUtilities::Vector3f GetWorldDirection() const;

private:
	Transform myTransform;
};
