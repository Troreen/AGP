#include "GraphicsEngine.pch.h"
#include "Buffer.h"

Buffer::Buffer() : mySize(0), myStride(0), myType(BufferType::Unknown)
{

}

Buffer::~Buffer() = default;
