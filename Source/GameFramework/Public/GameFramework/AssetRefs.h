#pragma once
#include <memory>
#include <string>
class Mesh;
class MaterialInterface;
namespace GameFrameworkInternal { class AssetAccess; }
namespace GameFrameworkIntegration { class AssetBindings; }

struct AssetId
{
    std::string Value;
    bool operator==(const AssetId&) const = default;
};
// Ready, retained resources. Gameplay can bind them, but cannot mutate their backend.
class MeshAsset
{
public:
    explicit operator bool() const { return bool(myResource); }
private:
    std::shared_ptr<::Mesh> myResource;
    friend class GameFrameworkInternal::AssetAccess;
    friend class GameFrameworkIntegration::AssetBindings;
};
class MaterialAsset
{
public:
    explicit operator bool() const { return bool(myResource); }
private:
    std::shared_ptr<::MaterialInterface> myResource;
    friend class GameFrameworkInternal::AssetAccess;
    friend class GameFrameworkIntegration::AssetBindings;
};
class AssetLookup
{
public:
    virtual ~AssetLookup() = default;
    virtual MeshAsset FindMesh(const AssetId&) const { return {}; }
    virtual MaterialAsset FindMaterial(const AssetId&) const { return {}; }
};
