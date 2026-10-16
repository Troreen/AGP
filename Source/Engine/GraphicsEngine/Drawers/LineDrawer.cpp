#include "GraphicsEngine.pch.h"
#include "LineDrawer.h"

LineDrawer& LineDrawer::Get()
{
	static LineDrawer instance;
	return instance;
}

void LineDrawer::DrawLine(const CU::Vector3f& aFrom, const CU::Vector3f& aTo, const CU::Vector4f aColor)
{
	CU::Vector4f from = aFrom.ToVector4(1.0f);
	CU::Vector4f to = aTo.ToVector4(1.0f);

	myLines.push_back({ from, aColor });
	myLines.push_back({ to, aColor });
}

const std::vector<SimpleVertex>& LineDrawer::GetLines() const
{
	return myLines;
}

void LineDrawer::Clear()
{
	myLines.clear();
}
