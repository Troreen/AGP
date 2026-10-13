#pragma once
#include <assert.h>
#include <vector>

namespace CommonUtilities
{
	template <typename T>
	class Stack
	{
	public:
		Stack();
		~Stack();

		int GetSize() const;
		const T& GetTop() const;
		T& GetTop();

		void Push(const T& aValue);
		T Pop();

	private:
		std::vector<T> myStack;
	};


	//Skapar en tom stack
	template <typename T>
	Stack<T>::Stack()
	{

	}

	//Frigör allt minne som stacken allokerat
	template <typename T>
	Stack<T>::~Stack()
	{
		
	}

	//Returnerar antal element i stacken
	template <typename T>
	int Stack<T>::GetSize() const
	{
		return static_cast<int>(myStack.size());
	}
	
	//Returnerar det översta elementet i stacken. Kraschar med en assert om
	//stacken är tom.
	template <typename T>
	const T& Stack<T>::GetTop() const
	{
		assert(myStack.size() > 0);
		int topElementIndex = static_cast<int>(myStack.size()) - 1;
		return myStack[topElementIndex];
	}
	
	//Returnerar det översta elementet i stacken. Kraschar med en assert om
	//stacken är tom.
	template <typename T>
	T& Stack<T>::GetTop()
	{
		assert(myStack.size() > 0);
		int topElementIndex = static_cast<int>(myStack.size()) - 1;
		return myStack[topElementIndex];
	}
	
	//Lägger in ett nytt element överst på stacken
	template <typename T>
	void Stack<T>::Push(const T& aValue)
	{
		myStack.push_back(aValue);
	}
	
	//Tar bort det översta elementet från stacken och returnerar det. Kraschar
	//med en assert om stacken är tom.
	template <typename T>
	T Stack<T>::Pop()
	{
		assert(myStack.size() > 0);
		int topElementIndex = static_cast<int>(myStack.size()) - 1;
		T value = myStack[topElementIndex];
		myStack.pop_back();
		return value;
	}
}