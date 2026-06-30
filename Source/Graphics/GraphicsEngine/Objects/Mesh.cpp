#include "GraphicsEngine.pch.h"
#include "Mesh.h"
#include "Vertex.h"

#include <algorithm>
#include <limits>

bool Skeleton::IsValid() const
{
    return !Joints.empty() && Joints.size() <= 128;
}

bool Animation::IsValid() const
{
    return !Frames.empty() && FramesPerSecond > 0.0f;
}

Mesh::Mesh() = default;
Mesh::~Mesh() = default;

void Mesh::Initialize(std::string_view aName, std::vector<Element>&& aElementList, 
    std::vector<Vertex>&& aVertexList, std::vector<unsigned>&& aIndexList)
{
    myName = aName;
    myElements = std::move(aElementList);
    myVertices = std::move(aVertexList);
    myIndices = std::move(aIndexList);

    unsigned highestMaterialIndex = 0;
    for (const Element& element : myElements)
    {
        if (element.MaterialIndex > highestMaterialIndex)
        {
            highestMaterialIndex = element.MaterialIndex;
        }
    }

	myNumMaterialSlots = myElements.empty() ? 0 : static_cast<size_t>(highestMaterialIndex) + 1;

    myHasLocalBounds = !myVertices.empty();
    if (myHasLocalBounds)
    {
        CU::Vector3f minBounds(
            (std::numeric_limits<float>::max)(),
            (std::numeric_limits<float>::max)(),
            (std::numeric_limits<float>::max)());
        CU::Vector3f maxBounds(
            (std::numeric_limits<float>::lowest)(),
            (std::numeric_limits<float>::lowest)(),
            (std::numeric_limits<float>::lowest)());

        for (const Vertex& vertex : myVertices)
        {
            const CU::Vector3f position(vertex.Position.x, vertex.Position.y, vertex.Position.z);
            minBounds.x = (std::min)(minBounds.x, position.x);
            minBounds.y = (std::min)(minBounds.y, position.y);
            minBounds.z = (std::min)(minBounds.z, position.z);
            maxBounds.x = (std::max)(maxBounds.x, position.x);
            maxBounds.y = (std::max)(maxBounds.y, position.y);
            maxBounds.z = (std::max)(maxBounds.z, position.z);
        }

        myLocalBoundsCenter = (minBounds + maxBounds) * 0.5f;
        myLocalBoundsRadius = 0.0f;
        for (const Vertex& vertex : myVertices)
        {
            const CU::Vector3f position(vertex.Position.x, vertex.Position.y, vertex.Position.z);
            myLocalBoundsRadius = (std::max)(myLocalBoundsRadius, (position - myLocalBoundsCenter).Length());
        }
    }
    else
    {
        myLocalBoundsCenter = CU::Vector3f::Zero;
        myLocalBoundsRadius = 0.0f;
    }
}

void Mesh::SetSkeleton(Skeleton&& aSkeleton)
{
    mySkeleton = std::move(aSkeleton);
}

const Skeleton* Mesh::GetSkeleton() const
{
    return HasSkeleton() ? &mySkeleton : nullptr;
}

bool Mesh::HasSkeleton() const
{
    return mySkeleton.IsValid();
}

void Mesh::AddAnimation(std::shared_ptr<Animation> anAnimation)
{
    if (anAnimation == nullptr || anAnimation->Name.empty() || !anAnimation->IsValid())
    {
        return;
    }

    myAnimations[anAnimation->Name] = std::move(anAnimation);
}

std::shared_ptr<Animation> Mesh::GetAnimation(std::string_view aName) const
{
    const auto foundAnimation = myAnimations.find(std::string(aName));
    if (foundAnimation == myAnimations.end())
    {
        return nullptr;
    }

    return foundAnimation->second;
}
