#pragma once
#include "GameFramework/AssetRefs.h"
namespace GameFrameworkInternal
{
    class AssetAccess
    {
    public:
        static std::shared_ptr<::Mesh> Mesh(const MeshAsset& value) { return value.myResource; }
        static std::shared_ptr<::MaterialInterface> Material(const MaterialAsset& value) { return value.myResource; }
        static MeshAsset WrapMesh(std::shared_ptr<::Mesh> value) { MeshAsset result; result.myResource = std::move(value); return result; }
        static MaterialAsset WrapMaterial(std::shared_ptr<::MaterialInterface> value) { MaterialAsset result; result.myResource = std::move(value); return result; }
    };
}
