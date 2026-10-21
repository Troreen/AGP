#pragma once
#include "BT_Node.h"
#include <Vector.hpp>
#include <memory>

class BlackBoard;
class BehaviourTree;
class Actor;
class ActorControllerData;

class BT_LeafNodeAction : public BT_Node
{
public:
	BT_LeafNodeAction() = delete;
	BT_LeafNodeAction(BlackBoard* aBlackBoard) { myBlackBoard = aBlackBoard; }

	virtual void UpdateActorControllerData() const {}
	virtual CommonUtilities::Vector2<float> GetSteeringForce(const Actor& /*aActor*/) const { return CommonUtilities::Vector2<float>(); }
	virtual void TriggerAction(Actor& /*aActor*/) const {}

protected:
	BlackBoard* myBlackBoard;
};
