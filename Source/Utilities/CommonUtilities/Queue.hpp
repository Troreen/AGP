#pragma once
#include <assert.h>
#include <array>

namespace CommonUtilities
{
	template <typename T>
	class Queue
	{
	public:
		Queue();
		~Queue();

		int GetSize() const;
		const T& GetFront() const;
		T& GetFront();
		bool Empty();

		void Enqueue(const T& aValue);
		T Dequeue();

	private:
		int myFirstInQueue;
		int myNextQueueTicket;
		int mySize;
		int myCapacity;
		T* myData;
	};

	//Skapar en tom kö
	template <typename T>
	Queue<T>::Queue()
	{
		myFirstInQueue = 0;
		myNextQueueTicket = 0;
		mySize = 0;
		myCapacity = 8;
		myData = new T[myCapacity];
	}

	//Frigör allt minne som Queue allokerat
	template <typename T>
	Queue<T>::~Queue()
	{
		delete[] myData;
		myData = nullptr;
	}

	//Returnerar antal element i kön
	template <typename T>
	int Queue<T>::GetSize() const
	{
		return mySize;
	}

	//Returnerar elementet längst fram i kön. Kraschar med en assert om kön är 
	//tom
	template <typename T>
	const T& Queue<T>::GetFront() const
	{
		assert(mySize > 0);
		return myData[myFirstInQueue];
	}

	//Returnerar elementet längst fram i kön. Kraschar med en assert om kön är tom
	template <typename T>
	T& Queue<T>::GetFront()
	{
		assert(mySize > 0);
		return myData[myFirstInQueue];
	}

	//Är kön tom
	template<typename T>
	inline bool Queue<T>::Empty()
	{
		return mySize == 0;
	}

	//Lägger in ett nytt element längst bak i kön
	template <typename T>
	void Queue<T>::Enqueue(const T& aValue)
	{
		myData[myNextQueueTicket] = aValue;
		mySize += 1;

		if (myNextQueueTicket == (myCapacity - 1))
		{
			myNextQueueTicket = 0;
		}
		else
		{
			myNextQueueTicket += 1;
		}

		if (mySize == myCapacity)
		{
			int index = 0;
			int oldCapacity = myCapacity;

			myCapacity *= 2;
			assert(myCapacity > oldCapacity);
			T* growingData = new T[myCapacity];

			for (int jindex = myFirstInQueue; jindex < oldCapacity; ++jindex)
			{
				growingData[index] = myData[jindex];
				++index;
			}
			for (int kindex = 0; kindex < myFirstInQueue; ++kindex)
			{
				growingData[index] = myData[kindex];
				++index;
			}
			myNextQueueTicket = index;
			myFirstInQueue = 0;

			delete[] myData;
			myData = growingData;
			growingData = nullptr;
		}
	}

	//Tar bort elementet längst fram i kön och returnerar det. Kraschar med en 
	//assert om kön är tom.
	template <typename T>
	T Queue<T>::Dequeue()
	{
		assert(mySize > 0);

		T returnValue = myData[myFirstInQueue];
		mySize -= 1;

		if (mySize == 0)
		{
			myFirstInQueue = 0;
		}
		else if (myFirstInQueue == (myCapacity - 1))
		{
			myFirstInQueue = 0;
		}
		else
		{
			myFirstInQueue += 1;
		}


		if (myCapacity > 8)
		{
			if (mySize <= (myCapacity / 4))
			{
				int index = 0;
				int oldCapacity = myCapacity;

				myCapacity /= 2;
				T* shrinkingData = new T[myCapacity];

				for (int jindex = myFirstInQueue; (jindex < oldCapacity) && (index < myCapacity); ++jindex)
				{
					shrinkingData[index] = myData[jindex];
					++index;
				}
				for (int kindex = 0; index < myCapacity; ++kindex)
				{
					shrinkingData[index] = myData[kindex];
					++index;
				}
				myNextQueueTicket = index;
				myFirstInQueue = 0;

				delete[] myData;
				myData = shrinkingData;
				shrinkingData = nullptr;
			}
		}
		return returnValue;
	}
}
