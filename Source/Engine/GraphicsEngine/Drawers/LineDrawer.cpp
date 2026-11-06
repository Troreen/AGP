#include "GraphicsEngine.pch.h"
#include "LineDrawer.h"

LineDrawer& LineDrawer::Get()
{
	static LineDrawer instance;
	return instance;
}

void LineDrawer::DrawLine([[maybe_unused]] const CU::Vector3f& aFrom, [[maybe_unused]] const CU::Vector3f& aTo, [[maybe_unused]] const CU::Vector4f aColor)
{
#ifndef _RETAIL
	CU::Vector4f from = aFrom.ToVector4(1.0f);
	CU::Vector4f to = aTo.ToVector4(1.0f);

	myLines.push_back({ from, aColor });
	myLines.push_back({ to, aColor });
#endif
}

const std::vector<SimpleVertex>& LineDrawer::GetLines() const
{
	return myLines;
}

void LineDrawer::Clear()
{
	myLines.clear();
}
