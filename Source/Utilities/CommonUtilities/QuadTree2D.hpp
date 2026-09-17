#pragma once

#include "Rect2D.hpp"

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace CommonUtilities
{
	class QuadTree2D
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
			std::vector<int> myObjectIndices;
			std::array<std::unique_ptr<Node>, 4> myChildren;
			int myDepth = 0;

			bool IsLeaf() const
			{
				return myChildren[0] == nullptr;
			}
		};

		void Init(const Rect2D& aWorldBounds, int aMaxObjectsPerNode = 4, int aMaxDepth = 8)
		{
			myObjects.clear();
			myMaxObjectsPerNode = aMaxObjectsPerNode;
			myMaxDepth = aMaxDepth;
			myRoot = std::make_unique<Node>();
			myRoot->myBounds = aWorldBounds;
		}

		void Clear()
		{
			if (!myRoot)
			{
				return;
			}

			const Rect2D worldBounds = myRoot->myBounds;
			Init(worldBounds, myMaxObjectsPerNode, myMaxDepth);
		}

		void Insert(int anObjectId, const Rect2D& aBounds)
		{
			if (!myRoot)
			{
				return;
			}

			myObjects.push_back({ anObjectId, aBounds });
			InsertObject(*myRoot, static_cast<int>(myObjects.size() - 1));
		}

		std::vector<int> GetObjectIdsAtPointWithParents(const Vector2<float>& aPoint) const
		{
			std::vector<int> objectIds;
			if (!myRoot || !myRoot->myBounds.Contains(aPoint))
			{
				return objectIds;
			}

			CollectObjectIdsAtPoint(*myRoot, aPoint, objectIds);
			return objectIds;
		}

		void VisitNodes(const std::function<void(const Node&)>& aVisitor) const
		{
			if (!myRoot)
			{
				return;
			}

			VisitNodesRecursive(*myRoot, aVisitor);
		}

		const std::vector<ObjectEntry>& GetObjects() const { return myObjects; }
		const Node* GetRoot() const { return myRoot.get(); }

	private:
		void InsertObject(Node& aNode, int anObjectIndex)
		{
			if (!aNode.IsLeaf())
			{
				const int childIndex = GetContainingChildIndex(aNode, myObjects[anObjectIndex].myBounds);
				if (childIndex != -1)
				{
					InsertObject(*aNode.myChildren[static_cast<size_t>(childIndex)], anObjectIndex);
					return;
				}
			}

			aNode.myObjectIndices.push_back(anObjectIndex);
			if (aNode.IsLeaf()
				&& static_cast<int>(aNode.myObjectIndices.size()) > myMaxObjectsPerNode
				&& aNode.myDepth < myMaxDepth)
			{
				Split(aNode);
			}
		}

		void Split(Node& aNode)
		{
			const Vector2<float> center = aNode.myBounds.GetCenter();
			const Rect2D& bounds = aNode.myBounds;

			aNode.myChildren[0] = CreateChild({ bounds.myMin.x, bounds.myMin.y }, { center.x, center.y }, aNode.myDepth + 1);
			aNode.myChildren[1] = CreateChild({ center.x, bounds.myMin.y }, { bounds.myMax.x, center.y }, aNode.myDepth + 1);
			aNode.myChildren[2] = CreateChild({ bounds.myMin.x, center.y }, { center.x, bounds.myMax.y }, aNode.myDepth + 1);
			aNode.myChildren[3] = CreateChild({ center.x, center.y }, { bounds.myMax.x, bounds.myMax.y }, aNode.myDepth + 1);

			std::vector<int> remainingObjectIndices;
			for (const int objectIndex : aNode.myObjectIndices)
			{
				const int childIndex = GetContainingChildIndex(aNode, myObjects[objectIndex].myBounds);
				if (childIndex == -1)
				{
					remainingObjectIndices.push_back(objectIndex);
					continue;
				}

				InsertObject(*aNode.myChildren[static_cast<size_t>(childIndex)], objectIndex);
			}

			aNode.myObjectIndices = remainingObjectIndices;
		}

		std::unique_ptr<Node> CreateChild(const Vector2<float>& aMin, const Vector2<float>& aMax, int aDepth) const
		{
			std::unique_ptr<Node> child = std::make_unique<Node>();
			child->myBounds = { aMin, aMax };
			child->myDepth = aDepth;
			return child;
		}

		int GetContainingChildIndex(const Node& aNode, const Rect2D& anObjectBounds) const
		{
			for (int childIndex = 0; childIndex < static_cast<int>(aNode.myChildren.size()); ++childIndex)
			{
				const std::unique_ptr<Node>& child = aNode.myChildren[static_cast<size_t>(childIndex)];
				if (child && child->myBounds.Contains(anObjectBounds))
				{
					return childIndex;
				}
			}

			return -1;
		}

		void CollectObjectIdsAtPoint(const Node& aNode, const Vector2<float>& aPoint, std::vector<int>& someObjectIds) const
		{
			for (const int objectIndex : aNode.myObjectIndices)
			{
				someObjectIds.push_back(myObjects[objectIndex].myId);
			}

			for (const std::unique_ptr<Node>& child : aNode.myChildren)
			{
				if (child && child->myBounds.Contains(aPoint))
				{
					CollectObjectIdsAtPoint(*child, aPoint, someObjectIds);
					return;
				}
			}
		}

		void VisitNodesRecursive(const Node& aNode, const std::function<void(const Node&)>& aVisitor) const
		{
			aVisitor(aNode);
			for (const std::unique_ptr<Node>& child : aNode.myChildren)
			{
				if (child)
				{
					VisitNodesRecursive(*child, aVisitor);
				}
			}
		}

		std::unique_ptr<Node> myRoot;
		std::vector<ObjectEntry> myObjects;
		int myMaxObjectsPerNode = 4;
		int myMaxDepth = 8;
	};
}
