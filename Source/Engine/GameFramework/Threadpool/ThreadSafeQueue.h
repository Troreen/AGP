#pragma once
#include "CommonUtilities/Queue.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace CommonUtilities
{
	template <typename T>
	class ThreadSafeQueue
	{
	public:
		~ThreadSafeQueue(void)
		{
			invalidate();
		}

		bool TryPop(T& out)
		{
			std::lock_guard<std::mutex> lock{ myMutex };
			if (myQueue.Empty() || !myValid)
			{
				return false;
			}
			out = std::move(myQueue.Dequeue());
			return true;
		}

	private:
		std::atomic_bool myValid{ true };
		mutable std::mutex myMutex;
		CommonUtilities::Queue<T> myQueue;
		std::condition_variable myCondition;
	};
}


