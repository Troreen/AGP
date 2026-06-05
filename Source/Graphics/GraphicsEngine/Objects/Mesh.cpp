#include "GraphicsEngine.pch.h"
#include "Mesh.h"
#include "Vertex.h"

Mesh::Mesh() = default;
Mesh::~Mesh() = default;

void Mesh::Initialize(std::string_view aName, std::vector<Element>&& aElementList, 
    std::vector<Vertex>&& aVertexList, std::vector<unsigned>&& aIndexList)
{
    myName = aName;
    myElements = std::move(aElementList);
    myVertices = std::move(aVertexList);
    myIndices = std::move(aIndexList);
}