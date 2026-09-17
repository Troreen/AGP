#pragma once
#include <cstdint>
#include <vector>
class Actor;
class Component;

namespace GameFrameworkInternal
{
	// Accessed only in serialized gameplay. Generation zero is permanently retired.
	struct ObjectSlots
	{
		struct Slot
		{
			Actor* actor = nullptr;
			Component* component = nullptr;
			uint64_t generation = 1;
		};

		std::vector<Slot> slots;
	};
}
