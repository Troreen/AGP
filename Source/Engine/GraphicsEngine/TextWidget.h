#pragma once

#include "GraphicsEngine/Objects/Buffer.h"
#include "GraphicsEngine/Objects/Font.h"
#include "GraphicsEngine/Objects/Vertex.h"
#include "Vector.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

class GraphicsEngine;

class TextWidget
{
public:
	void SetText(std::string_view text);
	void SetPosition(const CommonUtilities::Vector2f& position) { myPosition = position; }
	void SetPixelHeight(float height);
	void SetColor(const CommonUtilities::Vector4f& color);
	void SetOpacity(float opacity);
	void SetFont(std::shared_ptr<Font> font);

	const std::string& GetText() const { return myText; }
	const CommonUtilities::Vector2f& GetPosition() const { return myPosition; }
	float GetPixelHeight() const { return myPixelHeight; }
	float GetOpacity() const { return myOpacity; }
	const std::vector<Vertex>& GetVertices() const { return myVertices; }
	const std::vector<unsigned>& GetIndices() const { return myIndices; }
	unsigned GetGlyphCount() const { return myGlyphCount; }
	bool RebuildGeometry();

private:
	std::string myText;
	CommonUtilities::Vector2f myPosition = {0.0f, 0.0f};
	float myPixelHeight = 24.0f;
	CommonUtilities::Vector4f myColor = CommonUtilities::Vector4f::One;
	float myOpacity = 1.0f;
	std::shared_ptr<Font> myFont;
	std::vector<Vertex> myVertices;
	std::vector<unsigned> myIndices;
	mutable Buffer myVertexBuffer;
	mutable Buffer myIndexBuffer;
	bool myGeometryDirty = true;
	bool myGpuDirty = true;
	unsigned myGlyphCount = 0;

	friend class GraphicsEngine;
};
