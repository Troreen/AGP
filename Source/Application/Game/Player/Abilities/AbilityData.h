#pragma once


struct AbilityData
{
	float Cooldown = 5.0f;
	float Duration = 0.0f;
};

struct DashData : AbilityData
{
	float Distance;
	float Speed;
};
