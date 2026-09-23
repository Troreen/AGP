#pragma once
#include "BTNode.h"
#include "Actor.h"

class LeafNode : public BTNode
{
public:
	LeafNode(Actor& aActor) : myActor(aActor) {}
	~LeafNode() = default;

	virtual NodeStatus Tick() override {};

private:
	Actor& myActor;
};