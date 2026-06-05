#pragma once
#include "Buffer.h"
#include <vector>

struct Vertex;

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

private:

    std::string myName;
    std::vector<Vertex> myVertices;
    std::vector<unsigned> myIndices;
    std::vector<Element> myElements;

    mutable Buffer myVertexBuffer;
    mutable Buffer myIndexBuffer;
};