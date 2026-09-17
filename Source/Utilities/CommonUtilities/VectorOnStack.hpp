#pragma once

#include <assert.h>
#include <cstring>
#include <initializer_list>

namespace CommonUtilities
{
    template<typename T, int SIZE, typename CountType = unsigned short, bool UseSafeModeFlag = true>
    class VectorOnStack
    {
    public:
        VectorOnStack();
        VectorOnStack(const VectorOnStack& aVectorOnStack);
        VectorOnStack(const std::initializer_list<T>& aInitList);
        ~VectorOnStack();

        VectorOnStack& operator=(const VectorOnStack& aVectorOnStack);
        inline T& operator[](const CountType aIndex);
        inline const T& operator[](const CountType aIndex) const;

        inline void Add(const T& aObject);
        inline void Insert(const CountType aIndex, const T& aObject);

        inline void RemoveCyclic(const T& aObject);
        inline void RemoveCyclicAtIndex(const CountType aItemNumber);
        inline void Clear();

        __forceinline CountType Size() const;

    private:
        T myContainer[SIZE];

        CountType mySize;
    };

    template<typename T, int SIZE, typename CountType, bool UseSafeModeFlag>
    VectorOnStack<T, SIZE, CountType, UseSafeModeFlag>::VectorOnStack() : myContainer(), mySize(0)
    {
    }

    template<typename T, int SIZE, typename CountType, bool UseSafeModeFlag>
    VectorOnStack<T, SIZE, CountType, UseSafeModeFlag>::VectorOnStack(const VectorOnStack& aVectorOnStack) : myContainer()
    {
        mySize = aVectorOnStack.mySize;
        if (UseSafeModeFlag)
        {
            for (CountType itemIndex = 0; itemIndex < mySize; ++itemIndex)
            {
                myContainer[itemIndex] = aVectorOnStack.myContainer[itemIndex];
            }
        }
        else
        {
            std::memcpy(myContainer, aVectorOnStack.myContainer, sizeof(myContainer));
        }
    }

    template<typename T, int SIZE, typename CountType, bool UseSafeModeFlag>
    inline VectorOnStack<T, SIZE, CountType, UseSafeModeFlag>::VectorOnStack(const std::initializer_list<T>& aInitList) : myContainer()
    {
        assert(aInitList.size() <= SIZE && "To many items in the constructing initializer list.");

        if (UseSafeModeFlag)
        {
            CountType counter = 0;
            for (auto it = aInitList.begin(); it != aInitList.end(); ++it)
            {
                myContainer[counter] = *it;
                ++counter;
            }
            mySize = counter;
        }
        else
        {
            mySize = static_cast<CountType>(aInitList.size());
            std::memcpy(myContainer, aInitList.begin(), sizeof(T) * mySize);
        }
    }

    template<typename T, int SIZE, typename CountType, bool UseSafeModeFlag>
    VectorOnStack<T, SIZE, CountType, UseSafeModeFlag>::~VectorOnStack()
    {
        mySize = 0;
    }

    template<typename T, int SIZE, typename CountType, bool UseSafeModeFlag>
    inline VectorOnStack<T, SIZE, CountType, UseSafeModeFlag>& VectorOnStack<T, SIZE, CountType, UseSafeModeFlag>::operator=(const VectorOnStack& aVectorOnStack)
    {
        mySize = aVectorOnStack.mySize;
        if (UseSafeModeFlag)
        {
            for (CountType itemIndex = 0; itemIndex < mySize; ++itemIndex)
            {
                myContainer[itemIndex] = aVectorOnStack.myContainer[itemIndex];
            }
        }
        else
        {
            std::memcpy(myContainer, aVectorOnStack.myContainer, sizeof(myContainer));
        }

        return *this;
    }

    template<typename T, int SIZE, typename CountType, bool UseSafeModeFlag>
    inline T& VectorOnStack<T, SIZE, CountType, UseSafeModeFlag>::operator[](const CountType aIndex)
    {
        assert(aIndex >= 0 && aIndex < mySize && "The given index is outside the bounds of the array.");

        return myContainer[aIndex];
    }

    template<typename T, int SIZE, typename CountType, bool UseSafeModeFlag>
    inline const T& VectorOnStack<T, SIZE, CountType, UseSafeModeFlag>::operator[](const CountType aIndex) const
    {
        assert(aIndex >= 0 && aIndex < mySize && "The given index is outside the bounds of the array.");

        return myContainer[aIndex];
    }

    template<typename T, int SIZE, typename CountType, bool UseSafeModeFlag>
    inline void VectorOnStack<T, SIZE, CountType, UseSafeModeFlag>::Add(const T& aObject)
    {
        assert(mySize < SIZE && "This container is already full.");

        if (UseSafeModeFlag)
        {
            myContainer[mySize] = aObject;
        }
        else
        {
            std::memcpy(&myContainer[mySize], &aObject, sizeof(T));
        }
        ++mySize;
    }

    template<typename T, int SIZE, typename CountType, bool UseSafeModeFlag>
    inline void VectorOnStack<T, SIZE, CountType, UseSafeModeFlag>::Insert(const CountType aIndex, const T& aObject)
    {
        assert(mySize < SIZE && "This container is already full.");
        assert(aIndex >= 0 && aIndex <= mySize && "The given index is outside of the bounds of this array.");

        if (UseSafeModeFlag)
        {
            for (CountType index = mySize; index > aIndex; --index)
            {
                myContainer[index] = myContainer[index - 1];
            }
            myContainer[aIndex] = aObject;
        }
        else
        {
            std::memcpy(&myContainer[aIndex + 1], &myContainer[aIndex], sizeof(T) * (mySize - aIndex));
            std::memcpy(&myContainer[aIndex], &aObject, sizeof(T));
        }
        ++mySize;
    }

    template<typename T, int SIZE, typename CountType, bool UseSafeModeFlag>
    inline void VectorOnStack<T, SIZE, CountType, UseSafeModeFlag>::RemoveCyclic(const T& aObject)
    {
        for (CountType itemIndex = 0; itemIndex < mySize; ++itemIndex)
        {
            if (myContainer[itemIndex] == aObject)
            {
                RemoveCyclicAtIndex(itemIndex);
                return;
            }
        }
    }

    template<typename T, int SIZE, typename CountType, bool UseSafeModeFlag>
    inline void VectorOnStack<T, SIZE, CountType, UseSafeModeFlag>::RemoveCyclicAtIndex(const CountType aItemNumber)
    {
        assert(aItemNumber >= 0 && aItemNumber < mySize && "Given index is outside of the bounds of this container.");

        myContainer[aItemNumber] = myContainer[--mySize];
    }

    template<typename T, int SIZE, typename CountType, bool UseSafeModeFlag>
    inline void VectorOnStack<T, SIZE, CountType, UseSafeModeFlag>::Clear()
    {
        mySize = 0;
    }

    template<typename T, int SIZE, typename CountType, bool UseSafeModeFlag>
    inline CountType VectorOnStack<T, SIZE, CountType, UseSafeModeFlag>::Size() const
    {
        return mySize;
    }
}
