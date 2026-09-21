#include "MeshAsset.h"
#include "AssetRegistry.h"

bool MeshAsset::Load(const std::filesystem::path& path, AssetRegistry& registry)
{
	myMesh = registry.LoadMesh(path);
	return bool(myMesh);
}
