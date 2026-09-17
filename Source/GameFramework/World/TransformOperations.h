#pragma once
#include "Transform.hpp"
#include "ReparentMode.h"

namespace GameFrameworkInternal
{
    // Matrix decomposition lives in the engine implementation so gameplay headers
    // do not depend on DirectX. Failed conversions preserve the original pose.
    bool SetLocalMatrix(CommonUtilities::Transform& target, const CommonUtilities::Matrix4f& matrix);
    bool SetWorldMatrix(CommonUtilities::Transform& local, const CommonUtilities::Matrix4f& matrix);
    bool ChangeParent(CommonUtilities::Transform& local, CommonUtilities::Transform* parent, ReparentMode mode);
}
