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

	void Initialize();

	std::shared_ptr<Mesh> GetMesh(std::string_view aName) const;
	bool LoadFBXMesh(const std::filesystem::path& aPath);

private:
	void RegisterPrimitiveMeshes();
	void RegisterMesh(std::string aName, std::shared_ptr<Mesh> aMesh);
	std::filesystem::path ResolvePath(const std::filesystem::path& aPath) const;

	std::unordered_map<std::string, std::shared_ptr<Mesh>> myMeshes;
};
