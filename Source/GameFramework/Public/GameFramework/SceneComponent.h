#pragma once
#include "GameFramework/Component.h"
#include "GameFramework/Transform.h"
#include "GameFramework/ReparentMode.h"

// Spatial attachments are independent of behavior enablement. Local TRS is
// authored relative to a sibling spatial parent, or to the owning actor at a root.
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

	// World edits reject singular parents or local shear without changing the pose.
	bool SetWorldMatrix(const CommonUtilities::Matrix4f& matrix)
	{
		return myTransform.SetWorldMatrix(matrix);
	}

	CommonUtilities::Matrix4f GetWorldMatrix() const
	{
		return myTransform.GetWorldMatrix();
	}

	CommonUtilities::Vector3f GetWorldPosition() const;
	CommonUtilities::Vector3f GetWorldDirection() const;

	SceneComponent* GetParent() const
	{
		return myParent.Get();
	}

	// Applies immediately. Failure preserves both the previous parent and pose.
	bool SetParent(SceneComponent* parent, ReparentMode mode);

private:
	Transform myTransform;
	ComponentRef<SceneComponent> myParent;
	friend class World;
};
