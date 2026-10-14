#pragma once
#include "Asset.h"
#include <Vector3.hpp>
#include <AISystems/Pathfinder.hpp>
#include <Importer.h>

class NavMeshAsset : public Asset
{
	public:
		const std::vector<Pathfinder<float>::GraphNode>& GetNavGraph() const;

	protected:
		bool Load(const std::filesystem::path& aPath, AssetRegistry& aRegistry) override;

	private:
		std::vector<Pathfinder<float>::GraphNode> myNavGraph;

		CommonUtilities::Vector3f GetSwizzledPosition(const TGA::FBX::Vertex& aTgaFbxVertex) const;
};
