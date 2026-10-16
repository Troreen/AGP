#pragma once
#include <Vector.hpp>
#include <GraphicsEngine/Objects/Vertex.h>

class LineDrawer
{
public:
	static LineDrawer& Get();

	const std::vector<SimpleVertex>& GetLines() const;

	void DrawLine(const CU::Vector3f& aFrom, const CU::Vector3f& aTo, const CU::Vector4f aColor);
	void Clear();

private:

	LineDrawer() = default;

	std::vector<SimpleVertex> myLines;

};

