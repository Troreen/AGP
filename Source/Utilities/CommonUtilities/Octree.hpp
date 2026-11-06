// Orhan Nasufovski MMXXVI

#pragma once
#include "Vector3.hpp"
#include "Ray.hpp"
#include "AABB3D.hpp"
#include "Intersection.hpp"
#include <vector>
#include <utility>
#include <limits>
#include <algorithm>
#include <cstdint>
#include <cassert>

namespace CommonUtilities
{
	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE = 8, Type NODE_LOOSENESS = static_cast<Type>(1)>
	class Octree
	{
		public:
			Octree();
			~Octree();

			Octree(Octree&& anOther) noexcept;
			Octree& operator=(Octree&& anOther) noexcept;

			Octree(const Octree&) = delete;
			Octree& operator=(const Octree&) = delete;

			void Initialize(const CommonUtilities::Vector3<Type>& someMinCoords, const CommonUtilities::Vector3<Type>& someMaxCoords);
			void Query(const CommonUtilities::Vector3<Type>& aPosition, Type aHalfExtent, std::vector<EntityKey>& outResults) const;
			void Query(const CommonUtilities::Ray<Type>& aRay, std::vector<EntityKey>& outResults, Type aRayRadius = static_cast<Type>(0)) const;
			void Clear();

			bool QueryClosest(const CommonUtilities::Vector3<Type>& aPosition, EntityKey& outEntityKey) const;
			bool QueryClosest(const CommonUtilities::Ray<Type>& aRay, EntityKey& outEntityKey) const;
			bool Insert(const EntityKey& anEntityKey, const CommonUtilities::Vector3<Type>& aPosition, Type aHalfExtent);

		private:
			struct TreeItem
			{
				EntityKey entityKey;
				CommonUtilities::Vector3<Type> position;
				Type halfExtent;
			};

			std::vector<TreeItem> myItems;
			CommonUtilities::Vector3<Type> myMinCoords;
			CommonUtilities::Vector3<Type> myMaxCoords;
			Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>* myNodes;

			void SubdivideNode();
			void GetLooseBounds(CommonUtilities::Vector3<Type>& outMin, CommonUtilities::Vector3<Type>& outMax) const;
			bool QueryClosest(const CommonUtilities::Vector3<Type>& aPosition, EntityKey& outEntityKey, Type& outClosestDistSqr) const;
			bool QueryClosest(const CommonUtilities::Ray<Type>& aRay, EntityKey& outEntityKey, Type& outClosestDistSqr) const;
			bool GetFitsInNode(const CommonUtilities::Vector3<Type>& aPosition, Type aHalfExtent) const;
			bool GetOverlapsNode(const CommonUtilities::Vector3<Type>& aPosition, Type aHalfExtent) const;
			bool GetOverlapsNode(const CommonUtilities::Ray<Type>& aRay, Type aRayRadius) const;
			uint8_t GetSubdivisionSlot(const CommonUtilities::Vector3<Type>& aPosition) const;
	};

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::Octree() : myNodes{} {}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::~Octree()
	{
		Clear();
	}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::Octree(Octree&& anOther) noexcept
		: myItems(std::move(anOther.myItems))
		, myMinCoords(std::exchange(anOther.myMinCoords, CommonUtilities::Vector3<Type>()))
		, myMaxCoords(std::exchange(anOther.myMaxCoords, CommonUtilities::Vector3<Type>()))
		, myNodes(std::exchange(anOther.myNodes, nullptr))
	{}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>& Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::operator=(Octree&& anOther) noexcept
	{
		if (this != &anOther)
		{
			Clear();
			myItems = std::move(anOther.myItems);
			myMinCoords = std::exchange(anOther.myMinCoords, CommonUtilities::Vector3<Type>());
			myMaxCoords = std::exchange(anOther.myMaxCoords, CommonUtilities::Vector3<Type>());
			myNodes = std::exchange(anOther.myNodes, nullptr);
		}

		return *this;
	}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline void Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::Initialize(const CommonUtilities::Vector3<Type>& someMinCoords, const CommonUtilities::Vector3<Type>& someMaxCoords)
	{
		myMinCoords = someMinCoords;
		myMaxCoords = someMaxCoords;
	}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline void Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::Query(const CommonUtilities::Vector3<Type>& aPosition, Type aHalfExtent, std::vector<EntityKey>& outResults) const
	{
		if (!GetOverlapsNode(aPosition, aHalfExtent))
		{
			return;
		}

		for (const TreeItem& item : myItems)
		{
			outResults.push_back(item.entityKey);
		}

		if (myNodes == nullptr)
		{
			return;
		}

		for (int i = 0; i < 8; ++i)
		{
			myNodes[i].Query(aPosition, aHalfExtent, outResults);
		}
	}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline void Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::Query(const CommonUtilities::Ray<Type>& aRay, std::vector<EntityKey>& outResults, Type aRayRadius) const
	{
		if (!GetOverlapsNode(aRay, aRayRadius))
		{
			return;
		}

		for (const TreeItem& item : myItems)
		{
			outResults.push_back(item.entityKey);
		}

		if (myNodes == nullptr)
		{
			return;
		}

		for (int i = 0; i < 8; ++i)
		{
			myNodes[i].Query(aRay, outResults, aRayRadius);
		}
	}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline void Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::Clear()
	{
		delete[] myNodes;
		myNodes = nullptr;

		myItems.clear();
	}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline bool Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::QueryClosest(const CommonUtilities::Vector3<Type>& aPosition, EntityKey& outEntityKey) const
	{
		Type closestDistSqr = (std::numeric_limits<Type>::max)();
		return QueryClosest(aPosition, outEntityKey, closestDistSqr);
	}

	template<typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline bool Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::QueryClosest(const CommonUtilities::Ray<Type>& aRay, EntityKey& outEntityKey) const
	{
		Type closestDistSqr = (std::numeric_limits<Type>::max)();
		return QueryClosest(aRay, outEntityKey, closestDistSqr);
	}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline bool Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::Insert(const EntityKey& anEntityKey, const CommonUtilities::Vector3<Type>& aPosition, Type aHalfExtent)
	{
		assert(myMinCoords.x != myMaxCoords.x && myMinCoords.y != myMaxCoords.y && myMinCoords.z != myMaxCoords.z && "Octree hasn't been initialized with valid bounds");

		if (!GetFitsInNode(aPosition, aHalfExtent))
		{
			return false;
		}

		if (myNodes != nullptr)
		{
			Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>& childNode = myNodes[GetSubdivisionSlot(aPosition)];

			if (childNode.Insert(anEntityKey, aPosition, aHalfExtent))
			{
				return true;
			}
		}

		myItems.emplace_back(TreeItem{ anEntityKey, aPosition, aHalfExtent });

		if (myNodes == nullptr && MAX_ENTITIES_PER_NODE < myItems.size())
		{
			SubdivideNode();
		}

		return true;
	}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline void Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::SubdivideNode()
	{
		myNodes = new Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>[8];

		{
			const CommonUtilities::Vector3<Type> center = (myMinCoords + myMaxCoords) * static_cast<Type>(0.5);

			myNodes[0].Initialize(myMinCoords, center);
			myNodes[1].Initialize({ center.x, myMinCoords.y, myMinCoords.z }, { myMaxCoords.x, center.y, center.z });
			myNodes[2].Initialize({ myMinCoords.x, center.y, myMinCoords.z }, { center.x, myMaxCoords.y, center.z });
			myNodes[3].Initialize({ center.x, center.y, myMinCoords.z }, { myMaxCoords.x, myMaxCoords.y, center.z });
			myNodes[4].Initialize({ myMinCoords.x, myMinCoords.y, center.z }, { center.x, center.y, myMaxCoords.z });
			myNodes[5].Initialize({ center.x, myMinCoords.y, center.z }, { myMaxCoords.x, center.y, myMaxCoords.z });
			myNodes[6].Initialize({ myMinCoords.x, center.y, center.z }, { center.x, myMaxCoords.y, myMaxCoords.z });
			myNodes[7].Initialize(center, myMaxCoords);
		}

		for (uint32_t i = static_cast<uint32_t>(myItems.size()); 0 < i; --i)
		{
			const uint32_t index = i - 1;

			const TreeItem& item = myItems[index];
			Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>& childNode = myNodes[GetSubdivisionSlot(item.position)];

			if (childNode.Insert(item.entityKey, item.position, item.halfExtent))
			{
				myItems[index] = myItems.back();
				myItems.pop_back();
			}
		}
	}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline void Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::GetLooseBounds(CommonUtilities::Vector3<Type>& outMin, CommonUtilities::Vector3<Type>& outMax) const
	{
		const CommonUtilities::Vector3<Type> center = (myMinCoords + myMaxCoords) * static_cast<Type>(0.5);
		const CommonUtilities::Vector3<Type> halfExtent = (myMaxCoords - myMinCoords) * NODE_LOOSENESS * static_cast<Type>(0.5);
		outMin = center - halfExtent;
		outMax = center + halfExtent;
	}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline bool Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::QueryClosest(const CommonUtilities::Vector3<Type>& aPosition, EntityKey& outEntityKey, Type& outClosestDistSqr) const
	{
		const CommonUtilities::Vector3<Type> closestPointInNode
		{
			std::clamp(aPosition.x, myMinCoords.x, myMaxCoords.x),
			std::clamp(aPosition.y, myMinCoords.y, myMaxCoords.y),
			std::clamp(aPosition.z, myMinCoords.z, myMaxCoords.z)
		};

		if (outClosestDistSqr <= (closestPointInNode - aPosition).LengthSqr())
		{
			return false;
		}

		bool foundCloser = false;

		for (const TreeItem& item : myItems)
		{
			const Type distSqr = (item.position - aPosition).LengthSqr();

			if (distSqr < outClosestDistSqr)
			{
				outClosestDistSqr = distSqr;
				outEntityKey = item.entityKey;
				foundCloser = true;
			}
		}

		if (myNodes == nullptr)
		{
			return foundCloser;
		}

		const uint8_t ownSlot = GetSubdivisionSlot(aPosition);
		foundCloser |= myNodes[ownSlot].QueryClosest(aPosition, outEntityKey, outClosestDistSqr);

		for (uint8_t slot = 0; slot < 8; ++slot)
		{
			if (slot != ownSlot)
			{
				foundCloser |= myNodes[slot].QueryClosest(aPosition, outEntityKey, outClosestDistSqr);
			}
		}

		return foundCloser;
	}

	template<typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline bool Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::QueryClosest(const CommonUtilities::Ray<Type>& aRay, EntityKey& outEntityKey, Type& outClosestDistSqr) const
	{
		const CommonUtilities::Vector3<Type>& rayPos = aRay.GetOrigin();
		const CommonUtilities::Vector3<Type>& rayDir = aRay.GetDirection();
		const CommonUtilities::Vector3<Type> boundsCenter = (myMinCoords + myMaxCoords) * Type{ 0.5 };
		const CommonUtilities::Vector3<Type> rayToBoundsCenter = boundsCenter - rayPos;

		const Type boundsCenterAlongRayScalar = (std::max)(Type{ 0 }, rayDir.Dot(rayToBoundsCenter));

		const CommonUtilities::Vector3<Type> rayToBoundsCenterOffset = rayToBoundsCenter - rayDir * boundsCenterAlongRayScalar;

		{
			const Type sphereRadius = (myMaxCoords - boundsCenter).Length();
			const Type rayToSphereDist = rayToBoundsCenterOffset.Length() - sphereRadius;

			if (Type{ 0 } < rayToSphereDist && outClosestDistSqr <= rayToSphereDist * rayToSphereDist)
			{
				return false;
			}
		}

		bool foundCloser = false;

		for (const TreeItem& item : myItems)
		{
			const CommonUtilities::Vector3<Type> rayToItemPos = item.position - rayPos;
			const Type itemPosAlongRayScalar = (std::max)(Type{ 0 }, rayDir.Dot(rayToItemPos));

			const CommonUtilities::Vector3<Type> rayToItemPosOffset = rayToItemPos - rayDir * itemPosAlongRayScalar;
			const Type distSqr = rayToItemPosOffset.LengthSqr();

			if (distSqr < outClosestDistSqr)
			{
				outClosestDistSqr = distSqr;
				outEntityKey = item.entityKey;
				foundCloser = true;
			}
		}

		if (myNodes == nullptr)
		{
			return foundCloser;
		}

		const uint8_t firstSlot = GetSubdivisionSlot(boundsCenter - rayToBoundsCenterOffset);
		foundCloser |= myNodes[firstSlot].QueryClosest(aRay, outEntityKey, outClosestDistSqr);

		for (uint8_t slot = 0; slot < 8; ++slot)
		{
			if (slot != firstSlot)
			{
				foundCloser |= myNodes[slot].QueryClosest(aRay, outEntityKey, outClosestDistSqr);
			}
		}

		return foundCloser;
	}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline bool Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::GetFitsInNode(const CommonUtilities::Vector3<Type>& aPosition, Type aHalfExtent) const
	{
		if (aPosition.x < myMinCoords.x || myMaxCoords.x < aPosition.x || aPosition.y < myMinCoords.y || myMaxCoords.y < aPosition.y || aPosition.z < myMinCoords.z || myMaxCoords.z < aPosition.z)
		{
			return false;
		}

		CommonUtilities::Vector3<Type> looseMin;
		CommonUtilities::Vector3<Type> looseMax;
		GetLooseBounds(looseMin, looseMax);

		return looseMin.x <= (aPosition.x - aHalfExtent) && (aPosition.x + aHalfExtent) < looseMax.x && looseMin.y <= (aPosition.y - aHalfExtent) && (aPosition.y + aHalfExtent) < looseMax.y && looseMin.z <= (aPosition.z - aHalfExtent) && (aPosition.z + aHalfExtent) < looseMax.z;
	}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline bool Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::GetOverlapsNode(const CommonUtilities::Vector3<Type>& aPosition, Type aHalfExtent) const
	{
		CommonUtilities::Vector3<Type> looseMin;
		CommonUtilities::Vector3<Type> looseMax;
		GetLooseBounds(looseMin, looseMax);

		return looseMin.x <= (aPosition.x + aHalfExtent) && (aPosition.x - aHalfExtent) < looseMax.x && looseMin.y <= (aPosition.y + aHalfExtent) && (aPosition.y - aHalfExtent) < looseMax.y && looseMin.z <= (aPosition.z + aHalfExtent) && (aPosition.z - aHalfExtent) < looseMax.z;
	}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline bool Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::GetOverlapsNode(const CommonUtilities::Ray<Type>& aRay, Type aRayRadius) const
	{
		CommonUtilities::Vector3<Type> looseMin;
		CommonUtilities::Vector3<Type> looseMax;
		GetLooseBounds(looseMin, looseMax);

		const CommonUtilities::Vector3<Type>& rayPos = aRay.GetOrigin();
		const CommonUtilities::Vector3<Type>& rayDir = aRay.GetDirection();
		const CommonUtilities::Vector3<Type> radiusExtent{ aRayRadius, aRayRadius, aRayRadius };

		return CommonUtilities::IntersectionAABBRay(CommonUtilities::AABB3D<Type>{ looseMin - radiusExtent, looseMax + radiusExtent }, aRay);
	}

	template <typename Type, typename EntityKey, uint32_t MAX_ENTITIES_PER_NODE, Type NODE_LOOSENESS>
	inline uint8_t Octree<Type, EntityKey, MAX_ENTITIES_PER_NODE, NODE_LOOSENESS>::GetSubdivisionSlot(const CommonUtilities::Vector3<Type>& aPosition) const
	{
		const CommonUtilities::Vector3<Type> center = (myMinCoords + myMaxCoords) * static_cast<Type>(0.5);
		return (aPosition.x < center.x ? 0u : 1u) + (aPosition.y < center.y ? 0u : 2u) + (aPosition.z < center.z ? 0u : 4u);
	}
}
