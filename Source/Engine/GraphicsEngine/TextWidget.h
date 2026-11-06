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
	friend class GraphicsEngine;

public:
	void SetText(std::string_view text);
	void SetPosition(const CommonUtilities::Vector2f& position);
	void SetPixelHeight(float height);
	void SetColor(const CommonUtilities::Vector4f& color);
	void SetOpacity(float opacity);
	void SetFont(const std::shared_ptr<Font>& font);

	const std::string& GetText() const { return myText; }
	const CommonUtilities::Vector2f& GetPosition() const { return myPosition; }
	float GetPixelHeight() const { return myPixelHeight; }
	float GetOpacity() const { return myOpacity; }
	const std::vector<Vertex>& GetVertices() const { return myVertices; }
	const std::vector<unsigned>& GetIndices() const { return myIndices; }
	unsigned GetGlyphCount() const { return myGlyphCount; }

	bool RebuildGeometry();

private:
	mutable Buffer myVertexBuffer;
	mutable Buffer myIndexBuffer;

	std::string myText;

	std::vector<Vertex> myVertices;
	std::vector<unsigned> myIndices;
	std::shared_ptr<Font> myFont;

	CommonUtilities::Vector4f myColor = CommonUtilities::Vector4f::One;
	CommonUtilities::Vector2f myPosition = {0.0f, 0.0f};

	unsigned myGlyphCount = 0;

	float myPixelHeight = 24.0f;
	float myOpacity = 1.0f;

	bool myGeometryDirty = true;
	bool myGpuDirty = true;
};
