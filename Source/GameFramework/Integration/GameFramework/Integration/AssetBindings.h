#pragma once
#include "GameFramework/AssetRefs.h"
#include <unordered_map>
namespace GameFrameworkIntegration
{
    // A candidate-local view of ready resources from the existing asset backend.
    // Bind only immutable resources; create a fresh material instance for edits.
    class AssetBindings final : public AssetLookup
    {
    public:
        void BindMesh(const AssetId& id, std::shared_ptr<::Mesh> mesh)
        { MeshAsset value; value.myResource = std::move(mesh); myMeshes.insert_or_assign(id.Value, std::move(value)); }
        void BindMaterial(const AssetId& id, std::shared_ptr<::MaterialInterface> material)
        { MaterialAsset value; value.myResource = std::move(material); myMaterials.insert_or_assign(id.Value, std::move(value)); }
        MeshAsset FindMesh(const AssetId& id) const override
        { auto i = myMeshes.find(id.Value); return i == myMeshes.end() ? MeshAsset{} : i->second; }
        MaterialAsset FindMaterial(const AssetId& id) const override
        { auto i = myMaterials.find(id.Value); return i == myMaterials.end() ? MaterialAsset{} : i->second; }
    private:
        std::unordered_map<std::string, MeshAsset> myMeshes;
        std::unordered_map<std::string, MaterialAsset> myMaterials;
    };
}
