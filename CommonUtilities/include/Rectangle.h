#pragma once

namespace CommonUtilities
{
    template <typename T>
    struct Rectangle
    {
        T Top;
        T Left;
        T Bottom;
        T Right;
    };

    typedef Rectangle<float> Rectanglef;
}