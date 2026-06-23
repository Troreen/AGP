#include "PrimitiveMeshBuilder.h"

#include "GraphicsEngine/Objects/Mesh.h"
#include "GraphicsEngine/Objects/Vertex.h"

#include "Maths.hpp"

#include <cmath>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
	using Point3 = CommonUtilities::Vector3f;
	using Color = CommonUtilities::Vector4f;
	using UV = CommonUtilities::Vector2f;

	void FinalizeNormalsAndTangents(std::vector<Vertex>& inoutVertices, const std::vector<unsigned>& aIndices);

	const Color DefaultVertexColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	Vertex MakeVertex(const Point3& aPosition, const Color& aColor = DefaultVertexColor, const UV& aUV = { 0.0f, 0.0f })
	{
		Vertex vertex;
		vertex.Position = { aPosition.x, aPosition.y, aPosition.z, 1.0f };
		vertex.Color = aColor;
		vertex.UV0 = aUV;
		vertex.UV1 = aUV;
		return vertex;
	}

	std::shared_ptr<Mesh> CreateMesh(std::string_view aName, std::vector<Vertex>&& aVertices, std::vector<unsigned>&& aIndices)
	{
		FinalizeNormalsAndTangents(aVertices, aIndices);

		Mesh::Element element;
		element.NumVertices = static_cast<unsigned>(aVertices.size());
		element.NumIndices = static_cast<unsigned>(aIndices.size());

		auto mesh = std::make_shared<Mesh>();
		mesh->Initialize(aName, { element }, std::move(aVertices), std::move(aIndices));
		return mesh;
	}

	void AddQuad(std::vector<Vertex>& outVertices, std::vector<unsigned>& outIndices,
		const Point3& aPoint0, const Point3& aPoint1, const Point3& aPoint2, const Point3& aPoint3,
		const Color* aColorOverride = nullptr,
		const UV& aUV0 = { 0.0f, 1.0f }, const UV& aUV1 = { 1.0f, 1.0f }, const UV& aUV2 = { 1.0f, 0.0f }, const UV& aUV3 = { 0.0f, 0.0f });

	void AddBox(std::vector<Vertex>& outVertices, std::vector<unsigned>& outIndices,
		const Point3& aMin, const Point3& aMax, const Color& aColor)
	{
		AddQuad(outVertices, outIndices, Point3(aMin.x, aMin.y, aMin.z), Point3(aMax.x, aMin.y, aMin.z), Point3(aMax.x, aMax.y, aMin.z), Point3(aMin.x, aMax.y, aMin.z), &aColor);
		AddQuad(outVertices, outIndices, Point3(aMax.x, aMin.y, aMax.z), Point3(aMin.x, aMin.y, aMax.z), Point3(aMin.x, aMax.y, aMax.z), Point3(aMax.x, aMax.y, aMax.z), &aColor);
		AddQuad(outVertices, outIndices, Point3(aMin.x, aMin.y, aMax.z), Point3(aMin.x, aMin.y, aMin.z), Point3(aMin.x, aMax.y, aMin.z), Point3(aMin.x, aMax.y, aMax.z), &aColor);
		AddQuad(outVertices, outIndices, Point3(aMax.x, aMin.y, aMin.z), Point3(aMax.x, aMin.y, aMax.z), Point3(aMax.x, aMax.y, aMax.z), Point3(aMax.x, aMax.y, aMin.z), &aColor);
		AddQuad(outVertices, outIndices, Point3(aMin.x, aMax.y, aMin.z), Point3(aMax.x, aMax.y, aMin.z), Point3(aMax.x, aMax.y, aMax.z), Point3(aMin.x, aMax.y, aMax.z), &aColor);
		AddQuad(outVertices, outIndices, Point3(aMin.x, aMin.y, aMax.z), Point3(aMax.x, aMin.y, aMax.z), Point3(aMax.x, aMin.y, aMin.z), Point3(aMin.x, aMin.y, aMin.z), &aColor);
	}

	void AddTriangle(std::vector<Vertex>& outVertices, std::vector<unsigned>& outIndices,
		const Point3& aPoint0, const Point3& aPoint1, const Point3& aPoint2,
		const UV& aUV0 = { 0.0f, 1.0f }, const UV& aUV1 = { 0.5f, 0.0f }, const UV& aUV2 = { 1.0f, 1.0f })
	{
		const unsigned firstVertex = static_cast<unsigned>(outVertices.size());
		outVertices.push_back(MakeVertex(aPoint0, DefaultVertexColor, aUV0));
		outVertices.push_back(MakeVertex(aPoint1, DefaultVertexColor, aUV1));
		outVertices.push_back(MakeVertex(aPoint2, DefaultVertexColor, aUV2));

		outIndices.push_back(firstVertex);
		outIndices.push_back(firstVertex + 1);
		outIndices.push_back(firstVertex + 2);
	}

	void AddQuad(std::vector<Vertex>& outVertices, std::vector<unsigned>& outIndices,
		const Point3& aPoint0, const Point3& aPoint1, const Point3& aPoint2, const Point3& aPoint3,
		const Color* aColorOverride,
		const UV& aUV0, const UV& aUV1, const UV& aUV2, const UV& aUV3)
	{
		const unsigned firstVertex = static_cast<unsigned>(outVertices.size());
		const Color& color = aColorOverride ? *aColorOverride : DefaultVertexColor;
		outVertices.push_back(MakeVertex(aPoint0, color, aUV0));
		outVertices.push_back(MakeVertex(aPoint1, color, aUV1));
		outVertices.push_back(MakeVertex(aPoint2, color, aUV2));
		outVertices.push_back(MakeVertex(aPoint3, color, aUV3));

		outIndices.push_back(firstVertex);
		outIndices.push_back(firstVertex + 2);
		outIndices.push_back(firstVertex + 1);
		outIndices.push_back(firstVertex);
		outIndices.push_back(firstVertex + 3);
		outIndices.push_back(firstVertex + 2);
	}

	void AddReversedQuad(std::vector<Vertex>& outVertices, std::vector<unsigned>& outIndices,
		const Point3& aPoint0, const Point3& aPoint1, const Point3& aPoint2, const Point3& aPoint3,
		const UV& aUV0 = { 0.0f, 1.0f }, const UV& aUV1 = { 1.0f, 1.0f }, const UV& aUV2 = { 1.0f, 0.0f }, const UV& aUV3 = { 0.0f, 0.0f })
	{
		const unsigned firstVertex = static_cast<unsigned>(outVertices.size());
		outVertices.push_back(MakeVertex(aPoint0, DefaultVertexColor, aUV0));
		outVertices.push_back(MakeVertex(aPoint1, DefaultVertexColor, aUV1));
		outVertices.push_back(MakeVertex(aPoint2, DefaultVertexColor, aUV2));
		outVertices.push_back(MakeVertex(aPoint3, DefaultVertexColor, aUV3));

		outIndices.push_back(firstVertex);
		outIndices.push_back(firstVertex + 1);
		outIndices.push_back(firstVertex + 2);
		outIndices.push_back(firstVertex);
		outIndices.push_back(firstVertex + 2);
		outIndices.push_back(firstVertex + 3);
	}

	Point3 GetSpherePoint(float aLatitudeRadians, float aLongitudeRadians, float aRadius)
	{
		const float ringRadius = std::cos(aLatitudeRadians) * aRadius;
		return Point3(
			std::sin(aLongitudeRadians) * ringRadius,
			std::sin(aLatitudeRadians) * aRadius,
			std::cos(aLongitudeRadians) * ringRadius);
	}

	std::shared_ptr<Mesh> CreateSphereMesh(std::string_view aName, int aLatitudeSegments, int aLongitudeSegments, float aRadius)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned> indices;
		vertices.reserve(aLatitudeSegments * aLongitudeSegments * 4);
		indices.reserve(aLatitudeSegments * aLongitudeSegments * 6);

		for (int latitude = 0; latitude < aLatitudeSegments; ++latitude)
		{
			const float latitude0 = -CommonUtilities::Maths::HalfPi<float>() + CommonUtilities::Maths::Pi<float>() * static_cast<float>(latitude) / static_cast<float>(aLatitudeSegments);
			const float latitude1 = -CommonUtilities::Maths::HalfPi<float>() + CommonUtilities::Maths::Pi<float>() * static_cast<float>(latitude + 1) / static_cast<float>(aLatitudeSegments);
			const float v0 = 1.0f - static_cast<float>(latitude) / static_cast<float>(aLatitudeSegments);
			const float v1 = 1.0f - static_cast<float>(latitude + 1) / static_cast<float>(aLatitudeSegments);

			for (int longitude = 0; longitude < aLongitudeSegments; ++longitude)
			{
				const float longitude0 = CommonUtilities::Maths::TwoPi<float>() * static_cast<float>(longitude) / static_cast<float>(aLongitudeSegments);
				const float longitude1 = CommonUtilities::Maths::TwoPi<float>() * static_cast<float>(longitude + 1) / static_cast<float>(aLongitudeSegments);
				const float u0 = static_cast<float>(longitude) / static_cast<float>(aLongitudeSegments);
				const float u1 = static_cast<float>(longitude + 1) / static_cast<float>(aLongitudeSegments);
				AddReversedQuad(vertices, indices,
					GetSpherePoint(latitude0, longitude0, aRadius),
					GetSpherePoint(latitude0, longitude1, aRadius),
					GetSpherePoint(latitude1, longitude1, aRadius),
					GetSpherePoint(latitude1, longitude0, aRadius),
					UV{ u0, v0 },
					UV{ u1, v0 },
					UV{ u1, v1 },
					UV{ u0, v1 });
			}
		}

		return CreateMesh(aName, std::move(vertices), std::move(indices));
	}

	Point3 GetTorusPoint(float aMajorRadians, float aMinorRadians, float aMajorRadius, float aMinorRadius)
	{
		const float tubeX = aMajorRadius + std::cos(aMinorRadians) * aMinorRadius;
		return Point3(
			std::sin(aMajorRadians) * tubeX,
			std::sin(aMinorRadians) * aMinorRadius,
			std::cos(aMajorRadians) * tubeX);
	}

	Point3 GetPosition(const Vertex& aVertex)
	{
		return Point3(aVertex.Position.x, aVertex.Position.y, aVertex.Position.z);
	}

	Point3 GetFallbackTangent(const Point3& aNormal)
	{
		Point3 tangent = CommonUtilities::Vector3f::UnitY.Cross(aNormal);
		if (tangent.LengthSqr() <= 0.000001f)
		{
			tangent = CommonUtilities::Vector3f::UnitX.Cross(aNormal);
		}

		if (tangent.LengthSqr() <= 0.000001f)
		{
			return CommonUtilities::Vector3f::UnitX;
		}

		return tangent.GetNormalized();
	}

	void FinalizeNormalsAndTangents(std::vector<Vertex>& inoutVertices, const std::vector<unsigned>& aIndices)
	{
		for (Vertex& vertex : inoutVertices)
		{
			vertex.Normal = CommonUtilities::Vector3f::Zero;
			vertex.Tangent = CommonUtilities::Vector3f::Zero;
		}

		for (size_t index = 0; index + 2 < aIndices.size(); index += 3)
		{
			const unsigned i0 = aIndices[index + 0];
			const unsigned i1 = aIndices[index + 1];
			const unsigned i2 = aIndices[index + 2];
			if (i0 >= inoutVertices.size() || i1 >= inoutVertices.size() || i2 >= inoutVertices.size())
			{
				continue;
			}

			const Point3 p0 = GetPosition(inoutVertices[i0]);
			const Point3 p1 = GetPosition(inoutVertices[i1]);
			const Point3 p2 = GetPosition(inoutVertices[i2]);
			const UV uv0 = inoutVertices[i0].UV0;
			const UV uv1 = inoutVertices[i1].UV0;
			const UV uv2 = inoutVertices[i2].UV0;

			const Point3 edge1 = p1 - p0;
			const Point3 edge2 = p2 - p0;
			Point3 faceNormal = edge1.Cross(edge2);
			if (faceNormal.LengthSqr() <= 0.000001f)
			{
				faceNormal = CommonUtilities::Vector3f::UnitZ;
			}
			else
			{
				faceNormal.Normalize();
			}

			const UV deltaUV1 = uv1 - uv0;
			const UV deltaUV2 = uv2 - uv0;
			const float determinant = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;

			Point3 faceTangent = GetFallbackTangent(faceNormal);
			if (std::abs(determinant) > 0.000001f)
			{
				faceTangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) / determinant;
				if (faceTangent.LengthSqr() <= 0.000001f)
				{
					faceTangent = GetFallbackTangent(faceNormal);
				}
				else
				{
					faceTangent.Normalize();
				}
			}

			inoutVertices[i0].Normal += faceNormal;
			inoutVertices[i1].Normal += faceNormal;
			inoutVertices[i2].Normal += faceNormal;

			inoutVertices[i0].Tangent += faceTangent;
			inoutVertices[i1].Tangent += faceTangent;
			inoutVertices[i2].Tangent += faceTangent;
		}

		for (Vertex& vertex : inoutVertices)
		{
			if (vertex.Normal.LengthSqr() <= 0.000001f)
			{
				vertex.Normal = CommonUtilities::Vector3f::UnitZ;
			}
			else
			{
				vertex.Normal.Normalize();
			}

			vertex.Tangent = vertex.Tangent - vertex.Normal * vertex.Normal.Dot(vertex.Tangent);
			if (vertex.Tangent.LengthSqr() <= 0.000001f)
			{
				vertex.Tangent = GetFallbackTangent(vertex.Normal);
			}
			else
			{
				vertex.Tangent.Normalize();
			}
		}
	}
}

std::shared_ptr<Mesh> PrimitiveMeshBuilder::CreateFloor()
{
	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;

	AddQuad(vertices, indices,
		Point3(-0.5f, -0.5f, 0.0f),
		Point3(0.5f, -0.5f, 0.0f),
		Point3(0.5f, 0.5f, 0.0f),
		Point3(-0.5f, 0.5f, 0.0f));

	const unsigned firstVertex = static_cast<unsigned>(vertices.size());
	vertices.push_back(MakeVertex(Point3(-0.48f, -0.48f, 0.001f), DefaultVertexColor, UV{ 0.02f, 0.98f }));
	vertices.push_back(MakeVertex(Point3(0.48f, 0.48f, 0.001f), DefaultVertexColor, UV{ 0.98f, 0.02f }));
	vertices.push_back(MakeVertex(Point3(-0.48f, 0.48f, 0.001f), DefaultVertexColor, UV{ 0.02f, 0.02f }));
	vertices.push_back(MakeVertex(Point3(0.48f, -0.48f, 0.001f), DefaultVertexColor, UV{ 0.98f, 0.98f }));
	indices.insert(indices.end(), {
		firstVertex, firstVertex + 1, firstVertex + 2,
		firstVertex, firstVertex + 3, firstVertex + 1
	});

	return CreateMesh("Floor", std::move(vertices), std::move(indices));
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

	return CreateSphereMesh("Sphere", latitudeSegments, longitudeSegments, radius);
}

std::shared_ptr<Mesh> PrimitiveMeshBuilder::CreateSmoothSphere()
{
	constexpr int latitudeSegments = 24;
	constexpr int longitudeSegments = 48;
	constexpr float radius = 0.55f;

	return CreateSphereMesh("SmoothSphere", latitudeSegments, longitudeSegments, radius);
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
		const float u0 = static_cast<float>(major) / static_cast<float>(majorSegments);
		const float u1 = static_cast<float>(major + 1) / static_cast<float>(majorSegments);

		for (int minor = 0; minor < minorSegments; ++minor)
		{
			const float minor0 = CommonUtilities::Maths::TwoPi<float>() * static_cast<float>(minor) / static_cast<float>(minorSegments);
			const float minor1 = CommonUtilities::Maths::TwoPi<float>() * static_cast<float>(minor + 1) / static_cast<float>(minorSegments);
			const float v0 = static_cast<float>(minor) / static_cast<float>(minorSegments);
			const float v1 = static_cast<float>(minor + 1) / static_cast<float>(minorSegments);
			AddReversedQuad(vertices, indices,
				GetTorusPoint(major0, minor0, majorRadius, minorRadius),
				GetTorusPoint(major1, minor0, majorRadius, minorRadius),
				GetTorusPoint(major1, minor1, majorRadius, minorRadius),
				GetTorusPoint(major0, minor1, majorRadius, minorRadius),
				UV{ u0, v0 },
				UV{ u1, v0 },
				UV{ u1, v1 },
				UV{ u0, v1 });
		}
	}

	return CreateMesh("Torus", std::move(vertices), std::move(indices));
}

std::shared_ptr<Mesh> PrimitiveMeshBuilder::CreateAxes()
{
	constexpr float length = 500.0f;
	constexpr float halfThickness = 0.6f;

	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;
	vertices.reserve(72);
	indices.reserve(108);

	AddBox(vertices, indices,
		Point3(0.0f, -halfThickness, -halfThickness),
		Point3(length, halfThickness, halfThickness),
		Color{ 1.0f, 0.0f, 0.0f, 1.0f });

	AddBox(vertices, indices,
		Point3(-halfThickness, 0.0f, -halfThickness),
		Point3(halfThickness, length, halfThickness),
		Color{ 0.0f, 1.0f, 0.0f, 1.0f });

	AddBox(vertices, indices,
		Point3(-halfThickness, -halfThickness, 0.0f),
		Point3(halfThickness, halfThickness, length),
		Color{ 0.0f, 0.2f, 1.0f, 1.0f });

	return CreateMesh("WorldAxes", std::move(vertices), std::move(indices));
}
