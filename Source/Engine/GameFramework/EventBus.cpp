#include "EventBus.hpp"

EventBus::EventBus() : myListenerCreationCount(0) {}

bool EventBus::RemoveEventListener(ListenerID listenerID)
{
	auto it = myListenerIDToType.find(listenerID);

	if (it == myListenerIDToType.cend())
	{
		return false;
	}

	const std::type_index& typeIdx = it->second;
	
	if (auto listenerIt = myEventListenerLists.find(typeIdx); listenerIt != myEventListenerLists.cend())
	{
		bool isEmpty = false;
		listenerIt->second->Remove(listenerID, isEmpty);

		if (isEmpty)
		{
			myEventListenerLists.erase(listenerIt);
		}
	}

	myListenerIDToType.erase(it);
	return true;
}
