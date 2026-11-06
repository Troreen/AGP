#include "GraphicsEngine.pch.h"
#include "ParticleVertex.h"

const std::vector<VertexElementDesc> ParticleVertex::Description =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT },
    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT },
    { "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT },
    { "LIFETIME", 0, DXGI_FORMAT_R32_FLOAT },
    { "ANGLE", 0, DXGI_FORMAT_R32_FLOAT },
    { "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT },
    { "CHANNEL_MASK", 0, DXGI_FORMAT_R32G32B32A32_FLOAT }
};
