#pragma once
#include "Buffer.h"

#include <CommonUtilities/CommonUtilities.h>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

struct Vertex;

struct Skeleton
{
	struct Joint
	{
		CU::Matrix4f BindPoseInverse;
		int Parent = -1;
		std::vector<unsigned> Children;
		std::string Name;
	};

	std::vector<Joint> Joints;
	std::unordered_map<std::string, size_t> JointNameToIndex;

	bool IsValid() const;
};

class Mesh
{
    friend class GraphicsEngine;

public:
    struct Element 
    {
        unsigned VertexOffset = 0;
        unsigned IndexOffset = 0;
        unsigned NumVertices = 0;
        unsigned NumIndices = 0;
        unsigned MaterialIndex = 0;
    };

    Mesh() = default;
    ~Mesh() = default;

    Mesh& operator=(Mesh&) = delete;
    Mesh& operator=(Mesh&&) noexcept = default;

    void Initialize(std::string_view aName, std::vector<Element>&& aElementList, std::vector<Vertex>&& aVertexList, std::vector<unsigned>&& aIndexList);

	void SetSkeleton(Skeleton&& aSkeleton);
	const Skeleton* GetSkeleton() const;
	bool HasSkeleton() const;

    size_t GetNumMaterialSlots() const { return myNumMaterialSlots; }

private:
    Skeleton mySkeleton;

    std::string myName;
    std::vector<Vertex> myVertices;
    std::vector<unsigned> myIndices;
    std::vector<Element> myElements;

    size_t myNumMaterialSlots = 0;

	CU::Vector3f myLocalBoundsCenter = CU::Vector3f::Zero;
	float myLocalBoundsRadius = 0.0f;
	bool myHasLocalBounds = false;

    mutable Buffer myVertexBuffer;
    mutable Buffer myIndexBuffer;
};

