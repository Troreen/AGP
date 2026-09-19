#include "GraphicsEngine.pch.h"
#include "GraphicsEngine/TextWidget.h"

#include <algorithm>

namespace
{
	uint32_t DecodeUtf8(std::string_view text, size_t& offset)
	{
		const auto first = static_cast<unsigned char>(text[offset++]);
		if (first < 0x80) return first;
		unsigned extra = first >= 0xF0 ? 3u : first >= 0xE0 ? 2u : first >= 0xC0 ? 1u : 0u;
		uint32_t codepoint = extra == 3 ? first & 0x07u : extra == 2 ? first & 0x0Fu : extra == 1 ? first & 0x1Fu : 0u;
		if (extra == 0 || offset + extra > text.size()) return 0xFFFD;
		for (unsigned index = 0; index < extra; ++index)
		{
			const auto continuation = static_cast<unsigned char>(text[offset++]);
			if ((continuation & 0xC0u) != 0x80u) return 0xFFFD;
			codepoint = (codepoint << 6u) | (continuation & 0x3Fu);
		}
		return codepoint;
	}
}

void TextWidget::SetText(std::string_view text)
{
	if (myText == text) return;
	myText.assign(text);
	myGeometryDirty = true;
}

void TextWidget::SetPixelHeight(float height)
{
	height = (std::max)(0.0f, height);
	if (myPixelHeight == height) return;
	myPixelHeight = height;
	myGeometryDirty = true;
}

void TextWidget::SetColor(const CommonUtilities::Vector4f& color)
{
	if (myColor == color) return;
	myColor = color;
	myGeometryDirty = true;
}

void TextWidget::SetOpacity(float opacity)
{
	myOpacity = std::clamp(opacity, 0.0f, 1.0f);
}

void TextWidget::SetFont(std::shared_ptr<Font> font)
{
	if (myFont == font) return;
	myFont = std::move(font);
	myGeometryDirty = true;
}

bool TextWidget::RebuildGeometry()
{
	if (!myGeometryDirty) return true;
	myVertices.clear();
	myIndices.clear();
	myGlyphCount = 0;
	myGeometryDirty = false;
	myGpuDirty = true;
	if (!myFont || myText.empty() || myPixelHeight <= 0.0f) return bool(myFont) || myText.empty();

	float cursorX = 0.0f;
	float cursorY = 0.0f;
	size_t offset = 0;
	while (offset < myText.size())
	{
		uint32_t codepoint = DecodeUtf8(myText, offset);
		if (codepoint == '\r') continue;
		if (codepoint == '\n')
		{
			cursorX = 0.0f;
			cursorY += myFont->GetLineHeight() * myPixelHeight;
			continue;
		}
		const Font::Glyph* glyph = myFont->FindGlyph(codepoint);
		if (!glyph) glyph = myFont->FindGlyph('?');
		if (!glyph) continue;
		if (glyph->HasGeometry && codepoint != ' ')
		{
			const float left = cursorX + glyph->PlaneBounds.Left * myPixelHeight;
			const float right = cursorX + glyph->PlaneBounds.Right * myPixelHeight;
			const float top = cursorY + (glyph->PlaneBounds.Top - myFont->GetAscender()) * myPixelHeight;
			const float bottom = cursorY + (glyph->PlaneBounds.Bottom - myFont->GetAscender()) * myPixelHeight;
			const float u0 = glyph->AtlasBounds.Left / float(myFont->GetAtlasWidth());
			const float u1 = glyph->AtlasBounds.Right / float(myFont->GetAtlasWidth());
			const float v0 = glyph->AtlasBounds.Top / float(myFont->GetAtlasHeight());
			const float v1 = glyph->AtlasBounds.Bottom / float(myFont->GetAtlasHeight());
			const unsigned base = static_cast<unsigned>(myVertices.size());
			Vertex a, b, c, d;
			a.Position = {left, top, 0.0f, 1.0f}; a.UV0 = {u0, v0}; a.Color = myColor;
			b.Position = {right, top, 0.0f, 1.0f}; b.UV0 = {u1, v0}; b.Color = myColor;
			c.Position = {right, bottom, 0.0f, 1.0f}; c.UV0 = {u1, v1}; c.Color = myColor;
			d.Position = {left, bottom, 0.0f, 1.0f}; d.UV0 = {u0, v1}; d.Color = myColor;
			myVertices.insert(myVertices.end(), {a, b, c, d});
			myIndices.insert(myIndices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
			++myGlyphCount;
		}
		cursorX += glyph->Advance * myPixelHeight;
	}
	return true;
}
