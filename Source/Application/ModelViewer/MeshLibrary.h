#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

class Mesh;

class MeshLibrary
{
public:
	MeshLibrary();
	~MeshLibrary();

	void Initialize(const std::filesystem::path& aContentRoot);

	std::shared_ptr<Mesh> GetMesh(std::string_view aName) const;
	bool LoadFBXMesh(const std::filesystem::path& aPath);
	bool LoadFBXAnimation(std::string_view aMeshName, std::string aAnimationName, const std::filesystem::path& aPath);

private:
	void RegisterPrimitiveMeshes();
	void RegisterMesh(std::string aName, std::shared_ptr<Mesh> aMesh);
	std::filesystem::path ResolvePath(const std::filesystem::path& aPath) const;

	std::filesystem::path myContentRoot;
	std::unordered_map<std::string, std::shared_ptr<Mesh>> myMeshes;
};
