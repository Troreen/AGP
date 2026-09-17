#pragma once
#include "GameFramework/Registration/ComponentRegistry.h"

namespace GameFrameworkInternal
{
	class RegistryAccess
	{
	public:
		static void Freeze(ComponentRegistry& registry)
		{
			registry.Freeze();
		}
	};

	void RegisterBuiltInComponents(ComponentRegistry& registry);
}
