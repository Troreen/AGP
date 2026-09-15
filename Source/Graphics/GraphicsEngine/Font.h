#pragma once
#include "CommonUtilities/include/Rectangle.h"

class Font
{
    friend class GraphicsEngine;

public:
    struct Glyph
    {
        // The character code for this glyph.
        unsigned Unicode;
        // How much spacing we need for the next glyph.
        float Advance;

        // The size of the quad for this glyph, in font space.
        CU::Rectanglef PlaneBounds;
        // The UV coordinates for this glyph
        CU::Rectanglef AtlasBounds;
    };
private:
    // The height of a line of text, in font units.
    float myLineHeight;
    // The maximum heigh above the line a glyph can reach.
    float myAscender;
    // The maximum depth below the line a glyph can reach.
    float myDescender;

    std::unordered_map<char, Glyph> myGlyphs;
    Texture myAtlas;

public:
    const Glyph* GetGlyph(char aGlyph) const;
    float GetLineHeight() const { return myLineHeight; }
    float GetAscender() const { return myAscender; }
    float GetDescender() const { return myDescender; }
};

