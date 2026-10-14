#pragma once
#include <cassert>
#include <utility>
#include <memory>
#include <cstddef>

namespace CommonUtilities
{
	template <typename T, bool MIN_HEAP = false>
	class Heap
	{
		public:
			Heap();
			Heap(size_t anAllocationSize);
			~Heap();

			Heap(Heap&& anOther) noexcept;
			Heap& operator=(Heap&& anOther) noexcept;

			Heap(const Heap&) = delete;
			Heap& operator=(const Heap&) = delete;

			void Reserve(size_t anAllocationSize);
			void Clear();
			void Enqueue(T anElement);
			T Dequeue();

			const T& GetFront() const;
			size_t GetSize() const;

		private:
			T* myElements;
			size_t myAllocatedSize;
			size_t mySize;

			void MigrateToBiggerAllocation(size_t anAllocationSize);
			void ReleaseAllocation();
	};

	template <typename T, bool MIN_HEAP>
	inline Heap<T, MIN_HEAP>::Heap() : Heap(15) {}

	template <typename T, bool MIN_HEAP>
	inline Heap<T, MIN_HEAP>::Heap(size_t anAllocationSize)
		: myElements(std::allocator<T>{}.allocate(anAllocationSize))
		, myAllocatedSize(anAllocationSize)
		, mySize(0)
	{}

	template <typename T, bool MIN_HEAP>
	inline Heap<T, MIN_HEAP>::~Heap()
	{
		ReleaseAllocation();
	}

	template <typename T, bool MIN_HEAP>
	inline Heap<T, MIN_HEAP>::Heap(Heap&& anOther) noexcept
		: myElements(std::exchange(anOther.myElements, nullptr))
		, myAllocatedSize(std::exchange(anOther.myAllocatedSize, 0))
		, mySize(std::exchange(anOther.mySize, 0))
	{}

	template <typename T, bool MIN_HEAP>
	inline Heap<T, MIN_HEAP>& Heap<T, MIN_HEAP>::operator=(Heap&& anOther) noexcept
	{
		if (this != &anOther)
		{
			ReleaseAllocation();
			myElements = std::exchange(anOther.myElements, nullptr);
			myAllocatedSize = std::exchange(anOther.myAllocatedSize, 0);
			mySize = std::exchange(anOther.mySize, 0);
		}

		return *this;
	}

	template <typename T, bool MIN_HEAP>
	inline void Heap<T, MIN_HEAP>::Reserve(size_t anAllocationSize)
	{
		MigrateToBiggerAllocation(anAllocationSize);
	}

	template <typename T, bool MIN_HEAP>
	inline void Heap<T, MIN_HEAP>::Clear()
	{
		std::destroy_n(myElements, mySize);
		mySize = 0;
	}

	template <typename T, bool MIN_HEAP>
	inline void Heap<T, MIN_HEAP>::Enqueue(T anElement)
	{
		if (mySize == myAllocatedSize)
		{
			MigrateToBiggerAllocation(myAllocatedSize + (myAllocatedSize >> 2) + 1);
		}

		std::construct_at(myElements + mySize, std::move(anElement));
		size_t holeIndex = mySize++;

		T movingElement = std::move(myElements[holeIndex]);

		while (0 < holeIndex)
		{
			const size_t parentIndex = (holeIndex - 1) >> 1;

			if ((movingElement < myElements[parentIndex]) != MIN_HEAP)
			{
				break;
			}

			myElements[holeIndex] = std::move(myElements[parentIndex]);
			holeIndex = parentIndex;
		}

		myElements[holeIndex] = std::move(movingElement);
	}

	template <typename T, bool MIN_HEAP>
	inline T Heap<T, MIN_HEAP>::Dequeue()
	{
		assert(0 < mySize && "Tried to dequeue an element from an empty CommonUtilities::Heap");

		T frontElement = std::move(myElements[0]);
		
		if (--mySize == 0)
		{
			std::destroy_at(myElements);
			return frontElement;
		}

		T movingElement = std::move(myElements[mySize]);
		std::destroy_at(myElements + mySize);

		size_t holeIndex = 0;

		while (true)
		{
			const size_t childIndexA = (holeIndex << 1) + 1;

			if (mySize <= childIndexA)
			{
				break;
			}

			const size_t childIndexB = childIndexA + 1;
			const bool hasChildB = childIndexB < mySize;

			const size_t targetIndex = (hasChildB && (myElements[childIndexA] < myElements[childIndexB]) != MIN_HEAP)
				? childIndexB
				: childIndexA;

			if ((myElements[targetIndex] < movingElement) != MIN_HEAP)
			{
				break;
			}

			myElements[holeIndex] = std::move(myElements[targetIndex]);
			holeIndex = targetIndex;
		}

		myElements[holeIndex] = std::move(movingElement);
		return frontElement;
	}

	template <typename T, bool MIN_HEAP>
	inline const T& Heap<T, MIN_HEAP>::GetFront() const
	{
		assert(0 < mySize && "Tried to get an element from an empty CommonUtilities::Heap");
		return myElements[0];
	}

	template <typename T, bool MIN_HEAP>
	inline size_t Heap<T, MIN_HEAP>::GetSize() const
	{
		return mySize;
	}

	template <typename T, bool MIN_HEAP>
	inline void Heap<T, MIN_HEAP>::MigrateToBiggerAllocation(size_t anAllocationSize)
	{
		if (anAllocationSize <= myAllocatedSize)
		{
			return;
		}

		T* newElements = std::allocator<T>{}.allocate(anAllocationSize);

		std::uninitialized_move_n(myElements, mySize, newElements);

		ReleaseAllocation();
		myElements = newElements;
		myAllocatedSize = anAllocationSize;
	}

	template <typename T, bool MIN_HEAP>
	inline void Heap<T, MIN_HEAP>::ReleaseAllocation()
	{
		if (myElements != nullptr)
		{
			std::destroy_n(myElements, mySize);
			std::allocator<T>{}.deallocate(myElements, myAllocatedSize);
		}
	}
}
