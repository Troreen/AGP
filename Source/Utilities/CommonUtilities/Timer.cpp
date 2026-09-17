#include "Timer.h"

using Clock = std::chrono::high_resolution_clock;

CommonUtilities::Timer::Timer()
{
	myStartTime = Clock::now();
	myPrevTime = myStartTime;
	myCurrentTime = myStartTime;
	myDeltaTime = 0.0f;
}

void CommonUtilities::Timer::Update()
{
	myCurrentTime = Clock::now();
	auto frameDuration = std::chrono::duration<float>(myCurrentTime - myPrevTime);
	myDeltaTime = frameDuration.count();
	myPrevTime = myCurrentTime;
}

float CommonUtilities::Timer::GetDeltaTime() const
{
	return myDeltaTime;
}

double CommonUtilities::Timer::GetTotalTime() const
{
	auto totalDuration = std::chrono::duration<double>(myCurrentTime - myStartTime);
	return totalDuration.count();
}
