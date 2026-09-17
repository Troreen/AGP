#pragma once

#include "Rect2D.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

namespace CommonUtilities
{
	class KDTree2D
	{
	public:
		struct ObjectEntry
		{
			int myId = -1;
			Rect2D myBounds;
		};

		struct Node
		{
			Rect2D myBounds;
			std::unique_ptr<Node> myLeft;
			std::unique_ptr<Node> myRight;
			int myObjectIndex = -1;
			int myDepth = 0;
			int mySplitAxis = 0;
			float mySplitPosition = 0.0f;

			bool IsLeaf() const
			{
				return myLeft == nullptr && myRight == nullptr;
			}
		};

		void Build(const Rect2D& aWorldBounds, const std::vector<ObjectEntry>& someObjects)
		{
			myObjects = someObjects;
			std::vector<int> objectIndices;
			objectIndices.reserve(myObjects.size());
			for (int objectIndex = 0; objectIndex < static_cast<int>(myObjects.size()); ++objectIndex)
			{
				objectIndices.push_back(objectIndex);
			}

			myRoot = BuildRecursive(aWorldBounds, objectIndices, 0);
		}

		void VisitNodes(const std::function<void(const Node&)>& aVisitor) const
		{
			if (!myRoot)
			{
				return;
			}

			VisitNodesRecursive(*myRoot, aVisitor);
		}

		void VisitLeavesOverlapping(const Rect2D& aQueryBounds, const std::function<void(const Node&)>& aVisitor) const
		{
			if (!myRoot)
			{
				return;
			}

			VisitLeavesOverlappingRecursive(*myRoot, aQueryBounds, aVisitor);
		}

		const std::vector<ObjectEntry>& GetObjects() const { return myObjects; }

	private:
		std::unique_ptr<Node> BuildRecursive(const Rect2D& aNodeBounds, std::vector<int>& someObjectIndices, int aDepth)
		{
			std::unique_ptr<Node> node = std::make_unique<Node>();
			node->myBounds = aNodeBounds;
			node->myDepth = aDepth;
			node->mySplitAxis = aDepth % 2;

			if (someObjectIndices.empty())
			{
				return node;
			}

			if (someObjectIndices.size() == 1)
			{
				node->myObjectIndex = someObjectIndices[0];
				return node;
			}

			const int axis = node->mySplitAxis;
			std::sort(someObjectIndices.begin(), someObjectIndices.end(), [this, axis](int aLeft, int aRight)
			{
				return GetObjectCenterOnAxis(myObjects[aLeft], axis) < GetObjectCenterOnAxis(myObjects[aRight], axis);
			});

			const size_t middle = someObjectIndices.size() / 2;
			std::vector<int> leftIndices(someObjectIndices.begin(), someObjectIndices.begin() + middle);
			std::vector<int> rightIndices(someObjectIndices.begin() + middle, someObjectIndices.end());

			float splitPosition = (GetObjectCenterOnAxis(myObjects[leftIndices.back()], axis)
				+ GetObjectCenterOnAxis(myObjects[rightIndices.front()], axis)) * 0.5f;
			const float minSplit = axis == 0 ? aNodeBounds.myMin.x : aNodeBounds.myMin.y;
			const float maxSplit = axis == 0 ? aNodeBounds.myMax.x : aNodeBounds.myMax.y;
			if (splitPosition <= minSplit || splitPosition >= maxSplit)
			{
				splitPosition = (minSplit + maxSplit) * 0.5f;
			}

			node->mySplitPosition = splitPosition;
			Rect2D leftBounds = aNodeBounds;
			Rect2D rightBounds = aNodeBounds;
			if (axis == 0)
			{
				leftBounds.myMax.x = splitPosition;
				rightBounds.myMin.x = splitPosition;
			}
			else
			{
				leftBounds.myMax.y = splitPosition;
				rightBounds.myMin.y = splitPosition;
			}

			node->myLeft = BuildRecursive(leftBounds, leftIndices, aDepth + 1);
			node->myRight = BuildRecursive(rightBounds, rightIndices, aDepth + 1);
			return node;
		}

		float GetObjectCenterOnAxis(const ObjectEntry& anObject, int anAxis) const
		{
			const Vector2<float> center = anObject.myBounds.GetCenter();
			return anAxis == 0 ? center.x : center.y;
		}

		void VisitNodesRecursive(const Node& aNode, const std::function<void(const Node&)>& aVisitor) const
		{
			aVisitor(aNode);
			if (aNode.myLeft)
			{
				VisitNodesRecursive(*aNode.myLeft, aVisitor);
			}
			if (aNode.myRight)
			{
				VisitNodesRecursive(*aNode.myRight, aVisitor);
			}
		}

		void VisitLeavesOverlappingRecursive(const Node& aNode, const Rect2D& aQueryBounds, const std::function<void(const Node&)>& aVisitor) const
		{
			if (!aNode.myBounds.Intersects(aQueryBounds))
			{
				return;
			}

			if (aNode.IsLeaf())
			{
				aVisitor(aNode);
				return;
			}

			if (aNode.myLeft)
			{
				VisitLeavesOverlappingRecursive(*aNode.myLeft, aQueryBounds, aVisitor);
			}
			if (aNode.myRight)
			{
				VisitLeavesOverlappingRecursive(*aNode.myRight, aQueryBounds, aVisitor);
			}
		}

		std::unique_ptr<Node> myRoot;
		std::vector<ObjectEntry> myObjects;
	};
}
