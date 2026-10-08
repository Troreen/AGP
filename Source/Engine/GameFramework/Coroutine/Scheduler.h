#pragma once

#include <coroutine>
#include <chrono>
#include <functional>
#include <vector>

class Scheduler
{
public:
	using clock = std::chrono::steady_clock;

	void ResumeNextFrame(std::coroutine_handle<> aHandle);
	void ResumeAfter(std::coroutine_handle<> aHandle, std::coroutine_handle<> aAwaitHandle);
	void ResumeAfter(std::chrono::milliseconds aDelay, std::coroutine_handle<> aHandle);
	void ResumeEachFrameUntil(std::function<bool()> aCondition, std::coroutine_handle<> aHandle);

	void DeleteTimer(std::coroutine_handle<> aHandle);

	void Update();

	void IncreaseTimers(float aDeltaTime);

private:
	struct CoroutineTimer
	{
		clock::time_point time;
		std::coroutine_handle<> handle;
	};

	struct CoroutinePoller
	{
		std::function<bool()> condition;
		std::coroutine_handle<> handle;
	};

	inline static std::vector<std::coroutine_handle<>> myNext;
	inline static std::vector<CoroutineTimer> myTimers;
	inline static std::vector<CoroutinePoller> myPollers;
};
