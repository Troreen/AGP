#pragma once
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <typeindex>
#include <vector>
#include <memory>

class EventBus
{
	using ListenerID = uint32_t;

	public:
		EventBus();
		~EventBus() = default;

		template <typename EventType>
		void Dispatch(const EventType& anEvent);

		template <typename EventType>
		ListenerID AddEventListener(std::function<void(const EventType&)> aListener);

		bool RemoveEventListener(ListenerID listenerID);

	private:
		struct ListenersTypeWrapperBase
		{
			virtual ~ListenersTypeWrapperBase() = default;
			virtual bool Remove(ListenerID aListenerID, bool& anIsEmptyListFlag) = 0;
		};

		template <typename EventType>
		struct ListenersTypeWrapper : public ListenersTypeWrapperBase
		{
			std::vector<std::pair<ListenerID, std::function<void(const EventType&)>>> listeners;

			bool Remove(ListenerID aListenerID, bool& anIsEmptyListFlag) override
			{
				for (size_t i = 0; i < listeners.size(); ++i)
				{
					if (listeners[i].first == aListenerID)
					{
						listeners[i] = listeners.back();
						listeners.pop_back();
						anIsEmptyListFlag = listeners.empty();
						return true;
					}
				}

				return false;
			}
		};

		std::unordered_map<std::type_index, std::unique_ptr<ListenersTypeWrapperBase>> myEventListenerLists;
		std::unordered_map<ListenerID, std::type_index> myListenerIDToType;
		ListenerID myListenerCreationCount;
};

template<typename EventType>
inline void EventBus::Dispatch(const EventType& anEvent)
{
	const std::type_index typeIdx{ typeid(EventType) };

	if (auto it = myEventListenerLists.find(typeIdx); it != myEventListenerLists.cend())
	{
		ListenersTypeWrapper<EventType>& listenersTypeWrapper = *static_cast<ListenersTypeWrapper<EventType>*>(it->second.get());
		
		for (const auto& [ listenerID, listener ] : listenersTypeWrapper.listeners)
		{
			listener(anEvent);
		}
	}
}

template <typename EventType>
inline EventBus::ListenerID EventBus::AddEventListener(std::function<void(const EventType&)> aListener)
{
	const std::type_index typeIdx{ typeid(EventType) };

	auto [it, keyInserted] = myEventListenerLists.try_emplace(typeIdx, nullptr);

	if (keyInserted)
	{
		it->second = std::make_unique<ListenersTypeWrapper<EventType>>();
	}

	ListenersTypeWrapper<EventType>& listenersTypeWrapper = *static_cast<ListenersTypeWrapper<EventType>*>(it->second.get());
	listenersTypeWrapper.listeners.push_back(std::make_pair(++myListenerCreationCount, std::move(aListener)));
	myListenerIDToType.insert({ myListenerCreationCount, typeIdx });

	return myListenerCreationCount;
}
