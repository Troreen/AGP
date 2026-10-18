#include "GameFramework/EventBus.hpp"
#include <iostream>
#include <stdexcept>

namespace
{
	struct Notification {};

	void Check(bool aCondition, const char* aMessage)
	{
		if (!aCondition) { throw std::runtime_error(aMessage); }
	}

	void TestPublicListenerToken()
	{
		EventBus bus;
		const EventBus::ListenerID listenerId = bus.AddEventListener<Notification>([](const Notification&) {});
		Check(bus.RemoveEventListener(listenerId), "Public listener token cannot remove its subscription");
	}

	void TestRemovalBeforeAnotherListenersTurn()
	{
		EventBus bus;
		EventBus::ListenerID removedId = 0;
		int removedCalls = 0;
		int remainingCalls = 0;
		bus.AddEventListener<Notification>([&](const Notification&) { bus.RemoveEventListener(removedId); });
		removedId = bus.AddEventListener<Notification>([&](const Notification&) { ++removedCalls; });
		bus.AddEventListener<Notification>([&](const Notification&) { ++remainingCalls; });
		bus.Dispatch(Notification{});
		Check(removedCalls == 0 && remainingCalls == 1,
			"Removing a later listener must skip it without repeating or losing another listener");
	}

	void TestSelfRemovalAndNestedDispatch()
	{
		EventBus bus;
		EventBus::ListenerID selfId = 0;
		int selfCalls = 0;
		int addedCalls = 0;
		bool callbackFinished = false;
		selfId = bus.AddEventListener<Notification>([&](const Notification& anEvent)
		{
			++selfCalls;
			Check(bus.RemoveEventListener(selfId), "Executing listener could not remove itself");
			bus.AddEventListener<Notification>([&](const Notification&) { ++addedCalls; });
			bus.Dispatch(anEvent);
			callbackFinished = true;
		});
		bus.Dispatch(Notification{});
		Check(selfCalls == 1 && addedCalls == 1 && callbackFinished,
			"Self-removal of the last listener or a fresh nested subscription snapshot failed");
		bus.Dispatch(Notification{});
		Check(selfCalls == 1 && addedCalls == 2, "Removed listener returned on a later dispatch");
	}

	void TestNewListenerWaitsForNextDispatch()
	{
		EventBus bus;
		bool hasAdded = false;
		int addedCalls = 0;
		bus.AddEventListener<Notification>([&](const Notification&)
		{
			if (!hasAdded)
			{
				hasAdded = true;
				bus.AddEventListener<Notification>([&](const Notification&) { ++addedCalls; });
			}
		});
		bus.Dispatch(Notification{});
		Check(addedCalls == 0, "New listener joined the in-progress dispatch");
		bus.Dispatch(Notification{});
		Check(addedCalls == 1, "New listener missed the following dispatch");
	}

	void TestCallbackStatePersists()
	{
		EventBus bus;
		int observedCount = 0;
		bus.AddEventListener<Notification>([count = 0, &observedCount](const Notification&) mutable
		{
			observedCount = ++count;
		});
		bus.Dispatch(Notification{});
		bus.Dispatch(Notification{});
		Check(observedCount == 2, "Dispatch copied away mutable callback state");
	}

	void TestMissingListenersAndExceptions()
	{
		EventBus bus;
		bus.Dispatch(Notification{});
		Check(!bus.RemoveEventListener(12345), "Unknown listener removal should fail safely");
		const auto throwingId = bus.AddEventListener<Notification>([](const Notification&)
		{
			throw std::runtime_error("listener failure");
		});
		bool exceptionReachedCaller = false;
		try { bus.Dispatch(Notification{}); }
		catch (const std::runtime_error&) { exceptionReachedCaller = true; }
		Check(exceptionReachedCaller, "Dispatch swallowed a listener exception");
		Check(bus.RemoveEventListener(throwingId), "Listener could not be removed after an exception");
		bus.Dispatch(Notification{});
	}
}

int main()
{
	try
	{
		TestPublicListenerToken();
		TestRemovalBeforeAnotherListenersTurn();
		TestSelfRemovalAndNestedDispatch();
		TestNewListenerWaitsForNextDispatch();
		TestCallbackStatePersists();
		TestMissingListenersAndExceptions();
		std::cout << "EventBus tests passed\n";
		return 0;
	}
	catch (const std::exception& anException)
	{
		std::cerr << anException.what() << '\n';
		return 1;
	}
}
