#pragma once
#include "Component.h"
#include "../World/TransformOperations.h"

// Spatial attachments are independent of behavior enablement. Local TRS is
// authored relative to a sibling spatial parent, or to the owning actor at a root.
class SceneComponent : public Component
{
public:
    CommonUtilities::Transform& GetLocalTransform() { return myTransform; }
    const CommonUtilities::Transform& GetLocalTransform() const { return myTransform; }
    // World edits reject singular parents or local shear without changing the pose.
    bool SetWorldMatrix(const CommonUtilities::Matrix4f& matrix) { return GameFrameworkInternal::SetWorldMatrix(myTransform,matrix); }
    CommonUtilities::Matrix4f GetWorldMatrix() const { return myTransform.GetWorldMatrix(); }
    CommonUtilities::Vector3f GetWorldPosition() const;
    CommonUtilities::Vector3f GetWorldDirection() const;
    SceneComponent* GetParent() const { return myParent.Get(); }
    // During play, true means queued; boundary validation may still reject a request
    // if another structural change invalidated its target in the same frame.
    bool SetParent(SceneComponent* parent, ReparentMode mode);
private:
    bool ApplyParent(SceneComponent* parent, ReparentMode mode);
    CommonUtilities::Transform myTransform;
    ComponentHandle<SceneComponent> myParent;
    friend class World;
};
