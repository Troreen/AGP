#pragma once
#include <stdint.h>
#include <unordered_map>

class Actor;
class Component;

//enum class BlackBoardVariable_Bools
//{
//	eCount
//};
//
//enum class BlackBoardVariable_S32s
//{
//	eCount
//};
//
//enum class BlackBoardVariable_F32s
//{
//	eCount
//};

enum class BlackBoardVariable_Actors
{
	OwnerActor,
	eCount
};

enum class BlackBoardVariable_Components
{
	OwnerAnimation,
	eCount
};

class BlackBoard
{
public:
	//void SetBool(BlackBoardVariable_Bools aBoolVariable, bool aValue);
	//bool GetBool(BlackBoardVariable_Bools aBoolVariable);

	//void SetS32(BlackBoardVariable_S32s aS32Variable, uint32_t aValue);
	//uint32_t GetS32(BlackBoardVariable_S32s aS32Variable);

	//void SetF32(BlackBoardVariable_F32s aF32Variable, float aValue);
	//float GetF32(BlackBoardVariable_F32s aF32Variable);

	void SetActor(BlackBoardVariable_Actors aActorVariable, Actor* aValue);
	Actor* GetActor(BlackBoardVariable_Actors aActorVariable);

	void SetComponent(BlackBoardVariable_Components aComponentVariable, Component* aValue);
	Component* GetComponent(BlackBoardVariable_Components aComponentVariable);

private:
	std::unordered_map<BlackBoardVariable_Actors, Actor*> myActors;
	std::unordered_map<BlackBoardVariable_Components, Component*> myComponents;
};
