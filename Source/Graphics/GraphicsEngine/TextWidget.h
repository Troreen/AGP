#pragma once
#include "GraphicsEngine/Objects/Buffer.h"
#include "CUninUtilities/Math/Transform.h"

struct Vertex;

/// <summary>
/// Renders Screenspace Text
/// </summary>
class TextWidget
{
    friend class GraphicsEngine;

public:
    TextWidget(std::string_view aName);
    ~TextWidget();

    void SetText(std::string_view aText);
    void SetFont(const std::shared_ptr<Font>& aFont);
    void SetFontMaterial(const std::shared_ptr<Material>& aMaterial);
    void SetSize(float aPointSize);
    void SetZOrder(float aOrder);
    void SetDrawBackground(bool aDrawBackground);
    void SetColor(const CU::Vector4f& aColor);
    void SetBackgroundColor(const CU::Vector4f& aColor);

    void SetPosition(float X, float Y);

    CU::Matrix GetTextRenderMatrix() const;

private:
    bool RebuildGeometry();
    void RebuildTextRenderMatrix();

    // Screen Coords in Pixels
    CU::Vector2f myPosition;
    // Z Depth for Sorting (when on top / below other items). Not Depth Buffer Z!
    float myZOrder;
    // Font size in Points
    float mySize;

    CU::Vector4f myColor;
    CU::Vector4f myBackgroundColor;

    // Compiled matrix for rendering this text block in screen space.
    CU::Matrix myTextRenderMatrix;

    // Which MSDF font we're rendering.
    std::shared_ptr<Font> myFont;
    // And which material to render it with.
    std::shared_ptr<Material> myFontMaterial;

    // Vx / Ix buffers
    std::vector<Vertex> myVertices;
    std::vector<unsigned> myIndices;
    mutable Buffer myVertexBuffer;
    mutable Buffer myIndexBuffer;

    // If the Renderer should recreate the buffers before rendering or not.
    bool myIsDirty;

    // If we should draw a background quad for this text.
    bool myDrawBackground;

    // The name of this object, for easy inspection.
    std::string myName;
 
    // The actual text we are going to render
    std::string myTextString;
}