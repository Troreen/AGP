#pragma once
#ifndef THREADSAFEQUEUE_HPP
#define THREADSAFEQUEUE_HPP

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

		bool WaitPop(T& out)
		{
			std::unique_lock<std::mutex> lock{ myMutex };
			myCondition.wait(lock, [this]()
				{
					return !myQueue.Empty() || !myValid;
				});
			if (!myValid)
			{
				return false;
			}
			out = std::move(myQueue.Dequeue());
			return true;
		}

		void Push(T aValue)
		{
			std::lock_guard<std::mutex> lock{ myMutex };
			myQueue.Enqueue(std::move(aValue));
			myCondition.notify_one();
		}

		void Empty(void) const
		{
			std::lock_guard<std::mutex> lock{ myMutex };
			return myQueue.Empty();
		}

		void Clear(void)
		{
			std::lock_guard<std::mutex> lock{ myMutex };
			while (!myQueue.Empty())
			{
				myQueue.Dequeue();
			}
			myCondition.notify_all();
		}

		// Will invalidate this queue for all time and safely return stuck threads
		void Invalidate(void)
		{
			std::lock_guard<std::mutex> lock{ myMutex };
#if defined(_DEBUG)
			if (myValid == false)
			{
				// code should not use invalidate more than once
				__debugbreak();
			}
#endif
			myValid = false;
			myCondition.notify_all();
		}

		bool IsValid(void) const
		{
			std::lock_guard<std::mutex> lock{ myMutex };
			return myValid;
		}

	private:
		std::atomic_bool myValid{ true };
		mutable std::mutex myMutex;
		CommonUtilities::Queue<T> myQueue;
		std::condition_variable myCondition;
	};
}

#endif
