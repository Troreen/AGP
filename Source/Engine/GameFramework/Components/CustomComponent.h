#pragma once
#include "GameFramework/World/Component.h"

struct CustomComponentProperties;

class CustomComponent : public Component
{
public:
	virtual void Configure(const CustomComponentProperties& someProperties) = 0;
};
