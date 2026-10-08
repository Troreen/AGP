#pragma once

#include "Scheduler.h"

using namespace std::chrono;

void Scheduler::ResumeNextFrame(std::coroutine_handle<> aHandle)
{
	myNext.push_back(aHandle);
}

void Scheduler::ResumeAfter(std::coroutine_handle<> aHandle, std::coroutine_handle<> aAwaitHandle)
{
	myPollers.push_back(
		{
			[aAwaitHandle]() { return aAwaitHandle.done(); },
			aHandle
		});
}

void Scheduler::ResumeAfter(std::chrono::milliseconds aDelay, std::coroutine_handle<> aHandle)
{
	myTimers.push_back(
		{
			clock::now() + aDelay,
			aHandle
		});
}

void Scheduler::ResumeEachFrameUntil(std::function<bool()> aCondition, std::coroutine_handle<> aHandle)
{
	myPollers.push_back({ std::move(aCondition), aHandle });
}

void Scheduler::DeleteTimer(std::coroutine_handle<> aHandle)
{
	myTimers.erase(std::remove_if(myTimers.begin(), myTimers.end(),
			[aHandle](const CoroutineTimer& timer) { return timer.handle == aHandle; }),
		myTimers.end()
	);
}

void Scheduler::Update()
{
	std::vector<std::coroutine_handle<>> current(std::move(myNext));
	myNext.clear();

	for (std::coroutine_handle<>& handle : current)
	{
		handle.resume();
	}

	steady_clock::time_point now = clock::now();

	for (size_t timerIndex = 0; timerIndex < myTimers.size(); )
	{
		if (myTimers[timerIndex].time <= now)
		{
			std::coroutine_handle<> handle = myTimers[timerIndex].handle;
			myTimers.erase(myTimers.begin() + timerIndex);
			handle.resume();
		}
		else
		{
			++timerIndex;
		}
	}

	for (size_t pollerIndex = 0; pollerIndex < myPollers.size(); )
	{
		if (myPollers[pollerIndex].condition())
		{
			std::coroutine_handle<> handle = myPollers[pollerIndex].handle;
			myPollers.erase(myPollers.begin() + pollerIndex);
			handle.resume();
		}
		else
		{
			++pollerIndex;
		}
	}
}

void Scheduler::IncreaseTimers(float aDeltaTime)
{
	for (size_t timerIndex = 0; timerIndex < myTimers.size(); ++timerIndex)
	{
		myTimers[timerIndex].time += milliseconds(static_cast<long>(aDeltaTime * 1000));
	}
}
