#pragma once

#include <functional>
#include <vector>

using ListenerIdentification = size_t;

template <typename... Args>
class Event
{
public:
	using Callback = std::function<void(Args...)>;

	Event()
	{
		myNextIdentification = 0;
	}

	ListenerIdentification Subscribe(const Callback&& aCallback)
	{
		Listener newListener
		{
			.identification = GetANewIdentification(),
			.callback = std::move(aCallback),
		};

		myListeners.push_back(std::move(newListener));

		return myListeners.back().identification;
	}

	void Unsubscribe(ListenerIdentification aIdentification)
	{
		auto iterator = std::remove_if(myListeners.begin(), myListeners.end(),
			[aIdentification](const Listener& listener)
			{
				return listener.identification == aIdentification;
			});

		if (iterator != myListeners.end())
		{
			myFreedIdentifications.push_back(aIdentification);
			myListeners.erase(iterator, myListeners.end());
		}
	}

	void Clear()
	{
		myListeners.clear();
		myFreedIdentifications.clear();
		myNextIdentification = 0;
	}

	void Invoke(Args... args) const
	{
		for (size_t listenerIndex = 0; listenerIndex < myListeners.size(); listenerIndex++)
		{
			myListeners[listenerIndex].callback(args...);
		}
	}

	void operator()(Args... args) const
	{
		Invoke(args...);
	}

private:
	ListenerIdentification GetANewIdentification()
	{
		ListenerIdentification identification;

		if (myFreedIdentifications.empty())
		{
			identification = myNextIdentification++;
		}
		else
		{
			identification = myFreedIdentifications.back();
			myFreedIdentifications.pop_back();
		}

		return identification;
	}

	struct Listener
	{
		ListenerIdentification identification;
		Callback callback;
	};

	std::vector<Listener> myListeners;
	std::vector<ListenerIdentification> myFreedIdentifications;

	ListenerIdentification myNextIdentification;
};
