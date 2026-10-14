#pragma once
#include <stdint.h>
#include <unordered_map>

class Actor;

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

private:
	std::unordered_map<BlackBoardVariable_Actors, Actor*> myActors;
};
