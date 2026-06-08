#include "PrimitiveMeshBuilder.h"

#include "GraphicsEngine/Objects/Mesh.h"
#include "GraphicsEngine/Objects/Vertex.h"

#include "Maths.hpp"

#include <array>
#include <cmath>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
	using Point3 = CommonUtilities::Vector3f;
	using Color = CommonUtilities::Vector4f;

	const std::array<Color, 8> VertexColors = {
		Color{ 1.0f, 0.0f, 0.0f, 1.0f },
		Color{ 0.0f, 1.0f, 0.0f, 1.0f },
		Color{ 0.0f, 0.0f, 1.0f, 1.0f },
		Color{ 1.0f, 1.0f, 0.0f, 1.0f },
		Color{ 1.0f, 0.0f, 1.0f, 1.0f },
		Color{ 0.0f, 1.0f, 1.0f, 1.0f },
		Color{ 1.0f, 0.5f, 0.0f, 1.0f },
		Color{ 1.0f, 1.0f, 1.0f, 1.0f },
	};

	Vertex MakeVertex(const Point3& aPosition, const Color& aColor)
	{
		Vertex vertex;
		vertex.Position = { aPosition.x, aPosition.y, aPosition.z, 1.0f };
		vertex.Color = aColor;
		return vertex;
	}

	Color GetVertexColor(size_t anIndex)
	{
		return VertexColors[anIndex % VertexColors.size()];
	}

	std::shared_ptr<Mesh> CreateMesh(std::string_view aName, std::vector<Vertex>&& aVertices, std::vector<unsigned>&& aIndices)
	{
		Mesh::Element element;
		element.NumVertices = static_cast<unsigned>(aVertices.size());
		element.NumIndices = static_cast<unsigned>(aIndices.size());

		auto mesh = std::make_shared<Mesh>();
		mesh->Initialize(aName, { element }, std::move(aVertices), std::move(aIndices));
		return mesh;
	}

	void AddTriangle(std::vector<Vertex>& outVertices, std::vector<unsigned>& outIndices,
		const Point3& aPoint0, const Point3& aPoint1, const Point3& aPoint2)
	{
		const unsigned firstVertex = static_cast<unsigned>(outVertices.size());
		outVertices.push_back(MakeVertex(aPoint0, GetVertexColor(firstVertex)));
		outVertices.push_back(MakeVertex(aPoint1, GetVertexColor(firstVertex + 1)));
		outVertices.push_back(MakeVertex(aPoint2, GetVertexColor(firstVertex + 2)));

		outIndices.push_back(firstVertex);
		outIndices.push_back(firstVertex + 1);
		outIndices.push_back(firstVertex + 2);
	}

	void AddQuad(std::vector<Vertex>& outVertices, std::vector<unsigned>& outIndices,
		const Point3& aPoint0, const Point3& aPoint1, const Point3& aPoint2, const Point3& aPoint3)
	{
		const unsigned firstVertex = static_cast<unsigned>(outVertices.size());
		outVertices.push_back(MakeVertex(aPoint0, GetVertexColor(firstVertex)));
		outVertices.push_back(MakeVertex(aPoint1, GetVertexColor(firstVertex + 1)));
		outVertices.push_back(MakeVertex(aPoint2, GetVertexColor(firstVertex + 2)));
		outVertices.push_back(MakeVertex(aPoint3, GetVertexColor(firstVertex + 3)));

		outIndices.push_back(firstVertex);
		outIndices.push_back(firstVertex + 2);
		outIndices.push_back(firstVertex + 1);
		outIndices.push_back(firstVertex);
		outIndices.push_back(firstVertex + 3);
		outIndices.push_back(firstVertex + 2);
	}

	Point3 GetSpherePoint(float aLatitudeRadians, float aLongitudeRadians, float aRadius)
	{
		const float ringRadius = std::cos(aLatitudeRadians) * aRadius;
		return Point3(
			std::sin(aLongitudeRadians) * ringRadius,
			std::sin(aLatitudeRadians) * aRadius,
			std::cos(aLongitudeRadians) * ringRadius);
	}

	Point3 GetTorusPoint(float aMajorRadians, float aMinorRadians, float aMajorRadius, float aMinorRadius)
	{
		const float tubeX = aMajorRadius + std::cos(aMinorRadians) * aMinorRadius;
		return Point3(
			std::sin(aMajorRadians) * tubeX,
			std::sin(aMinorRadians) * aMinorRadius,
			std::cos(aMajorRadians) * tubeX);
	}
}

std::shared_ptr<Mesh> PrimitiveMeshBuilder::CreatePlane()
{
	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;

	AddQuad(vertices, indices,
		Point3(-0.5f, -0.5f, 0.0f),
		Point3(0.5f, -0.5f, 0.0f),
		Point3(0.5f, 0.5f, 0.0f),
		Point3(-0.5f, 0.5f, 0.0f));

	const unsigned firstVertex = static_cast<unsigned>(vertices.size());
	vertices.push_back(MakeVertex(Point3(-0.48f, -0.48f, 0.001f), GetVertexColor(firstVertex)));
	vertices.push_back(MakeVertex(Point3(0.48f, 0.48f, 0.001f), GetVertexColor(firstVertex + 1)));
	vertices.push_back(MakeVertex(Point3(-0.48f, 0.48f, 0.001f), GetVertexColor(firstVertex + 2)));
	vertices.push_back(MakeVertex(Point3(0.48f, -0.48f, 0.001f), GetVertexColor(firstVertex + 3)));
	indices.insert(indices.end(), {
		firstVertex, firstVertex + 1, firstVertex + 2,
		firstVertex, firstVertex + 3, firstVertex + 1
	});

	return CreateMesh("Plane", std::move(vertices), std::move(indices));
}

std::shared_ptr<Mesh> PrimitiveMeshBuilder::CreateCube()
{
	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;

	AddQuad(vertices, indices, Point3(-0.5f, -0.5f, -0.5f), Point3(0.5f, -0.5f, -0.5f), Point3(0.5f, 0.5f, -0.5f), Point3(-0.5f, 0.5f, -0.5f));
	AddQuad(vertices, indices, Point3(0.5f, -0.5f, 0.5f), Point3(-0.5f, -0.5f, 0.5f), Point3(-0.5f, 0.5f, 0.5f), Point3(0.5f, 0.5f, 0.5f));
	AddQuad(vertices, indices, Point3(-0.5f, -0.5f, 0.5f), Point3(-0.5f, -0.5f, -0.5f), Point3(-0.5f, 0.5f, -0.5f), Point3(-0.5f, 0.5f, 0.5f));
	AddQuad(vertices, indices, Point3(0.5f, -0.5f, -0.5f), Point3(0.5f, -0.5f, 0.5f), Point3(0.5f, 0.5f, 0.5f), Point3(0.5f, 0.5f, -0.5f));
	AddQuad(vertices, indices, Point3(-0.5f, 0.5f, -0.5f), Point3(0.5f, 0.5f, -0.5f), Point3(0.5f, 0.5f, 0.5f), Point3(-0.5f, 0.5f, 0.5f));
	AddQuad(vertices, indices, Point3(-0.5f, -0.5f, 0.5f), Point3(0.5f, -0.5f, 0.5f), Point3(0.5f, -0.5f, -0.5f), Point3(-0.5f, -0.5f, -0.5f));

	return CreateMesh("Cube", std::move(vertices), std::move(indices));
}

std::shared_ptr<Mesh> PrimitiveMeshBuilder::CreatePyramid()
{
	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;

	const Point3 p0(-0.5f, -0.5f, -0.5f);
	const Point3 p1(0.5f, -0.5f, -0.5f);
	const Point3 p2(0.5f, -0.5f, 0.5f);
	const Point3 p3(-0.5f, -0.5f, 0.5f);
	const Point3 top(0.0f, 0.5f, 0.0f);

	AddQuad(vertices, indices, p3, p2, p1, p0);
	AddTriangle(vertices, indices, p0, top, p1);
	AddTriangle(vertices, indices, p1, top, p2);
	AddTriangle(vertices, indices, p2, top, p3);
	AddTriangle(vertices, indices, p3, top, p0);

	return CreateMesh("Pyramid", std::move(vertices), std::move(indices));
}

std::shared_ptr<Mesh> PrimitiveMeshBuilder::CreateSphere()
{
	constexpr int latitudeSegments = 8;
	constexpr int longitudeSegments = 16;
	constexpr float radius = 0.55f;

	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;
	vertices.reserve(latitudeSegments * longitudeSegments * 4);
	indices.reserve(latitudeSegments * longitudeSegments * 6);

	for (int latitude = 0; latitude < latitudeSegments; ++latitude)
	{
		const float latitude0 = -CommonUtilities::Maths::HalfPi<float>() + CommonUtilities::Maths::Pi<float>() * static_cast<float>(latitude) / static_cast<float>(latitudeSegments);
		const float latitude1 = -CommonUtilities::Maths::HalfPi<float>() + CommonUtilities::Maths::Pi<float>() * static_cast<float>(latitude + 1) / static_cast<float>(latitudeSegments);

		for (int longitude = 0; longitude < longitudeSegments; ++longitude)
		{
			const float longitude0 = CommonUtilities::Maths::TwoPi<float>() * static_cast<float>(longitude) / static_cast<float>(longitudeSegments);
			const float longitude1 = CommonUtilities::Maths::TwoPi<float>() * static_cast<float>(longitude + 1) / static_cast<float>(longitudeSegments);
			AddQuad(vertices, indices,
				GetSpherePoint(latitude0, longitude0, radius),
				GetSpherePoint(latitude0, longitude1, radius),
				GetSpherePoint(latitude1, longitude1, radius),
				GetSpherePoint(latitude1, longitude0, radius));
		}
	}

	return CreateMesh("Sphere", std::move(vertices), std::move(indices));
}

std::shared_ptr<Mesh> PrimitiveMeshBuilder::CreateTorus()
{
	constexpr int majorSegments = 18;
	constexpr int minorSegments = 8;
	constexpr float majorRadius = 0.38f;
	constexpr float minorRadius = 0.17f;

	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;
	vertices.reserve(majorSegments * minorSegments * 4);
	indices.reserve(majorSegments * minorSegments * 6);

	for (int major = 0; major < majorSegments; ++major)
	{
		const float major0 = CommonUtilities::Maths::TwoPi<float>() * static_cast<float>(major) / static_cast<float>(majorSegments);
		const float major1 = CommonUtilities::Maths::TwoPi<float>() * static_cast<float>(major + 1) / static_cast<float>(majorSegments);

		for (int minor = 0; minor < minorSegments; ++minor)
		{
			const float minor0 = CommonUtilities::Maths::TwoPi<float>() * static_cast<float>(minor) / static_cast<float>(minorSegments);
			const float minor1 = CommonUtilities::Maths::TwoPi<float>() * static_cast<float>(minor + 1) / static_cast<float>(minorSegments);
			AddQuad(vertices, indices,
				GetTorusPoint(major0, minor0, majorRadius, minorRadius),
				GetTorusPoint(major1, minor0, majorRadius, minorRadius),
				GetTorusPoint(major1, minor1, majorRadius, minorRadius),
				GetTorusPoint(major0, minor1, majorRadius, minorRadius));
		}
	}

	return CreateMesh("Torus", std::move(vertices), std::move(indices));
}
