#pragma once
#include "../../../Assets/AssetRefs.h"
#include "../../../Runtime/Internal/AssetAccess.h"
#include <unordered_map>
namespace GameFrameworkIntegration
{
    // A candidate-local view of ready resources from the existing asset backend.
    // Bind only immutable resources; create a fresh material instance for edits.
    class AssetBindings final : public AssetLookup
    {
    public:
        void BindMesh(const AssetId& id, std::shared_ptr<::Mesh> mesh)
        { myMeshes.insert_or_assign(id.Value, GameFrameworkInternal::AssetAccess::WrapMesh(std::move(mesh))); }
        void BindMaterial(const AssetId& id, std::shared_ptr<::MaterialInterface> material)
        { myMaterials.insert_or_assign(id.Value, GameFrameworkInternal::AssetAccess::WrapMaterial(std::move(material))); }
        MeshAsset FindMesh(const AssetId& id) const override
        { auto i = myMeshes.find(id.Value); return i == myMeshes.end() ? MeshAsset{} : i->second; }
        MaterialAsset FindMaterial(const AssetId& id) const override
        { auto i = myMaterials.find(id.Value); return i == myMaterials.end() ? MaterialAsset{} : i->second; }
    private:
        std::unordered_map<std::string, MeshAsset> myMeshes;
        std::unordered_map<std::string, MaterialAsset> myMaterials;
    };
}
