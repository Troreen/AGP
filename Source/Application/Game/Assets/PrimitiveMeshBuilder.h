#pragma once

#include <memory>

class Mesh;

class PrimitiveMeshBuilder
{
public:
	static std::shared_ptr<Mesh> CreateFloor();
	static std::shared_ptr<Mesh> CreateCube();
	static std::shared_ptr<Mesh> CreatePyramid();
	static std::shared_ptr<Mesh> CreateSphere();
	static std::shared_ptr<Mesh> CreateSmoothSphere();
	static std::shared_ptr<Mesh> CreateTorus();
	static std::shared_ptr<Mesh> CreateAxes();
};
