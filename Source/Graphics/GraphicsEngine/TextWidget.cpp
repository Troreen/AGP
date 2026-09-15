#include "TextWidget.h"

TextWidget::TextWidget(std::string_view aName)
{
    myName = aName;
}

TextWidget::~TextWidget()
{
}

void TextWidget::SetText(std::string_view aText)
{
    myTextString = aText;
}

void TextWidget::SetFont(const std::shared_ptr<Font> &aFont)
{
    myFont = aFont;
}

void TextWidget::SetFontMaterial(const std::shared_ptr<Material> &aMaterial)
{
    myFontMaterial = aMaterial;
}

void TextWidget::SetSize(float aPointSize)
{
    mySize = aPointSize;
}

void TextWidget::SetZOrder(float aOrder)
{
    myZOrder = aOrder;
}

void TextWidget::SetDrawBackground(bool aDrawBackground)
{
    myDrawBackground = aDrawBackground;
}

void TextWidget::SetColor(const CU::Vector4f &aColor)
{
    myColor = aColor;
}

void TextWidget::SetBackgroundColor(const CU::Vector4f &aColor)
{
    myBackgroundColor = aColor;
}

void TextWidget::SetPosition(float X, float Y)
{
    myPosition = {X, Y};
    RebuildTextRenderMatrix();
}

CU::Matrix TextWidget::GetTextRenderMatrix() const
{
    return myTextRenderMatrix;
}

bool TextWidget::RebuildGeometry()
{
    using namespace CommonUtilities;

    myVertices.clear();
    myIndices.clear();

    if (myTextString. empty()) // TODO: flush buffers if empty string
        return true;    

    myVertices.reserve(myTextString.size() * 4 + 4);
    myIndices.reserve(myTextString.size() * 4 + 4);

    /*Background quad*/ //TODO: implement background quad rendering

    // Calculate the offset of each line. This assumes Top Left origin.
    // and -Y is Down.
    const Vector4f lineOffset = Vector4f(0, -myFont->GetLineHeight(), 0, 0);

    // Where we're drawing currently.
    Vector4f cursor = Vector4f::Zero;

    // Cache the color of the glyphs.
    Vector4f color = myColor;

    // Which text row we're drawing. To support newline characters.
    unsigned row = 0;

    // Go through the Text
    //for (char c : myTextString)
    for (size_t p =0; p < myTextString.size(); ++p)
    {
        const char c = myTextString[p];
        // Look for special characters.
        switch (c)
        {
        case '\n':
            {   
                ++row;
                cursor = Vector4f::Zero;
                cursor.Y = lineOffset.Y * row;
                continue;
            }
        case ' ':
            if (const Font :: Glyph* glyph = myFont->GetGlyph(c))
            {
                const Vector4f advance = Vector4f(glyph->Advance, 0, 0, 0);
                cursor += advance;
            }
            continue;
        }
        // Magical Things Which Are Not Required (coloring text, setting vertex colors based on some things etc.)
        
        if (const Font :: Glyph* glyph = myFont->GetGLyph(c))
        {
            const unsigned currentVxCount = static_cast<unsigned>(myVertices.size());

            Vertex v1, v2, v3, v4;
            v1.Position = Vector4f(glyph->PlaneBounds.Left, glyph->PlaneBounds. Top, @, 1) + cursor + lineOffset;
            v1.UV0 = {glyph->AtlasBounds.Left, 1- glyph->AtlasBounds. Top };
            v1.Color = color;
            v1.BoneIDs.X = 0; // I use this to tell the Glyph and Background vertices apart. You don't need it for just text.

            v2.Position = Vector4f(glyph->PlaneBounds.Right, glyph->PlaneBounds. Top, 0, 1) + cursor + lineOffset;
            v2.UV0 = {glyph->AtlasBounds.Right, 1 - glyph->AtlasBounds. Top };
            v2.Color = color;
            v2.BoneIDs.X = 0;

            v3.Position = Vector4f(glyph->PlaneBounds.Right, glyph->PlaneBounds.Bottom, 0, 1) + cursor + lineOffset;
            v3.UV0 = {glyph->AtlasBounds.Right, 1 - glyph->AtlasBounds.Bottom };
            v3.Color = color;
            v3.BoneIDs.X =0;

            v4.Position = Vector4f(glyph->PlaneBounds.Left, glyph->PlaneBounds.Bottom, 0, 1) + cursor + LineOffset;
            v4.UV0 = { glyph->AtlasBounds.Left, 1 - glyph->AtlasBounds.Bottom };
            v4.Color = color;
            v4.BoneIDs.X = 0;

            if (v2.Position.X > textBounds.X)
                textBounds.X = v2.Position.X;   

            if (v2.Position. Y < textBounds.Y)
                textBounds.Y = v2.Position. Y;

            myVertices.emplace_back(v1);
            myVertices.emplace_back(v2);
            myVertices.emplace_back(v3);
            myVertices.emplace_back(v4);

            myIndices.emplace_back(currentVxCount + 0);
            myIndices.emplace_back(currentVxCount + 1);
            myIndices.emplace_back(currentVxCount + 2);
            myIndices.emplace_back(currentVxCount + 0);
            myIndices.emplace_back(currentVxCount + 2);
            myIndices.emplace_back(currentVxCount + 3);

            const Vector4f advance = Vector4f(glyph->Advance, 0, 0, 0);
            cursor += advance;
        }
    }

    /*Background quad implementation*/ //TODO: implement background quad rendering

    return true;
}

void TextWidget::RebuildTextRenderMatrix()
{
    // Derive pixel size from points and dpi.
    const float pointSize = mySize; // mySize is font size in points.
    const float dpi = 96.0f;        // DPI scaling for fonts. 96 is default in Windows and other OS.
    const float fontPixelHeight = pointSize * (dpi / 72.0f);
 
    myTextRenderMatrix = CU::Matrix::FromScale(fontPixelHeight, -fontPixelHeight, 1.0f) * CU::Matrix::FromTranslation(myPosition.X, myPosition.Y, 0);
}