#pragma once
#include "GameFramework/Registration/ComponentRegistry.h"
namespace GameFrameworkInternal
{
    struct RegistryAccess
    {
        static void Freeze(ComponentRegistry& registry) { registry.Freeze(); }
    };
    void RegisterBuiltInComponents(ComponentRegistry& registry);
}
