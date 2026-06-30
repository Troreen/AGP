#pragma once
#include "Buffer.h"
#include "Matrix.hpp"
#include "Vector.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct Vertex;

struct Skeleton
{
    struct Joint
    {
        CU::Matrix4f BindPoseInverse;
        int Parent = -1;
        std::vector<int> Children;
        std::string Name;
    };

    std::vector<Joint> Joints;
    std::unordered_map<std::string, size_t> JointNameToIndex;

    bool IsValid() const;
};

struct Animation
{
    struct Frame
    {
        std::unordered_map<std::string, CU::Matrix4f> Transforms;
    };

    std::vector<Frame> Frames;
    float Duration = 0.0f;
    float FramesPerSecond = 0.0f;
    std::string Name;

    bool IsValid() const;
};

class Mesh
{
    friend class GraphicsEngine;

public:
    struct Element
    {
        // The start position in the Vertex Array / Buffer
        unsigned VertexOffset = 0;
        // The start position in the Index Array / Buffer
        unsigned IndexOffset = 0;
        // How many vertices this Element is.
        unsigned NumVertices = 0;
        // How many indices this Element is.
        unsigned NumIndices = 0;
        // The Material Index of this Element.
        unsigned MaterialIndex = 0;
    };

    Mesh();
    ~Mesh();

    Mesh operator=(Mesh& other) = delete;
    Mesh operator=(Mesh&& other) noexcept = delete;

    void Initialize(std::string_view aName, std::vector<Element>&& aElementList, 
        std::vector<Vertex>&& aVertexList, std::vector<unsigned>&& aIndexList);

    void SetSkeleton(Skeleton&& aSkeleton);
    const Skeleton* GetSkeleton() const;
    bool HasSkeleton() const;

    void AddAnimation(std::shared_ptr<Animation> anAnimation);
    std::shared_ptr<Animation> GetAnimation(std::string_view aName) const;
    
	std::string_view GetName() const { return myName; }
	size_t GetNumMaterialSlots() const { return myNumMaterialSlots; }

private:

    std::string myName;
    std::vector<Vertex> myVertices;
    std::vector<unsigned> myIndices;
    std::vector<Element> myElements;
    Skeleton mySkeleton;
    std::unordered_map<std::string, std::shared_ptr<Animation>> myAnimations;

	size_t myNumMaterialSlots = 0; 
	CU::Vector3f myLocalBoundsCenter = CU::Vector3f::Zero;
	float myLocalBoundsRadius = 0.0f;
	bool myHasLocalBounds = false;

    mutable Buffer myVertexBuffer;
    mutable Buffer myIndexBuffer;
};
