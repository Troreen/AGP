#pragma once

namespace GameFrameworkInternal
{
	class TimeAccess;
}

// Seconds in the current gameplay frame/phase. Loading time never advances these.
class GameTime
{
public:
	float GetDeltaTime() const
	{
		return myDelta;
	}

	float GetFixedDeltaTime() const
	{
		return myFixedDelta;
	}

	double GetElapsedTime() const
	{
		return myElapsed;
	}

private:
	float myDelta = 0;
	float myFixedDelta = 1.0f / 60.0f;
	double myElapsed = 0;
	friend class GameFrameworkInternal::TimeAccess;
};
