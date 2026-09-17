#pragma once
#include "../GameTime.h"
namespace GameFrameworkInternal
{
    class TimeAccess
    {
    public:
        static void Initialize(GameTime& time, float fixed) { time.myFixedDelta = fixed; }
        static void Fixed(GameTime& time, float delta) { time.myDelta = delta; }
        static void Frame(GameTime& time, float delta) { time.myDelta = delta; time.myElapsed += delta; }
        static void ResetTransient(GameTime& time) { time.myDelta = 0; }
    };
}
