#pragma once
#include "GameFramework/GameInput.h"

namespace GameFrameworkInternal
{
    // Only the host/scheduler consumes and combines callback samples.
    class InputAccess
    {
    public:
        static void ClearPressed(GameInput& input) { input.ClearPressed(); }
        static void Merge(GameInput& input, const GameInput& newer) { input.Merge(newer); }
    };
}
