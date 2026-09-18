#include "UnrealSceneImporter.h"
#include <SimdJson/simdjson.h>

#include <stdexcept>

namespace
{
	using Element = simdjson::dom::element;
	using Object = simdjson::dom::object;

	std::string ReadString(const Object& object, const char* key)
	{
		return std::string(std::string_view(object[key].get_string().value()));
	}

	std::string ReadOptionalString(const Object& object, const char* key)
	{
		const auto field = object[key];
		return field.error() == simdjson::NO_SUCH_FIELD || field.is_null()
			? std::string{}
			: std::string(std::string_view(field.get_string().value()));
	}

	float ReadNumber(const Object& object, const char* key)
	{
		return static_cast<float>(object[key].get_double().value());
	}

	CommonUtilities::Matrix4f ReadMatrix(const Element& element)
	{
		const auto values = element.get_array().value();
		if (values.size() != 16)
		{
			throw std::runtime_error("matrix must contain 16 numbers");
		}

		// UnrealSceneAdapter owns axis mapping, unit conversion and matrix decomposition.
		// Keeping the exported matrix unchanged here prevents those rules being duplicated
		// in every component branch, as they were in the first integration implementation.
		CommonUtilities::Matrix4f result;
		size_t index = 0;
		for (const auto value : values)
		{
			result(static_cast<int>(index / 4 + 1), static_cast<int>(index % 4 + 1)) =
				static_cast<float>(value.get_double().value());
			++index;
		}
		return result;
	}

	CommonUtilities::Vector3f ReadVector3(const Element& element)
	{
		const auto values = element.get_array().value();
		if (values.size() != 3)
		{
			throw std::runtime_error("vector must contain 3 numbers");
		}
		return {
			static_cast<float>(values.at(0).get_double().value()),
			static_cast<float>(values.at(1).get_double().value()),
			static_cast<float>(values.at(2).get_double().value())};
	}

	CommonUtilities::Vector3f ReadVector3Object(const Object& object)
	{
		return {ReadNumber(object, "x"), ReadNumber(object, "y"), ReadNumber(object, "z")};
	}

	CommonUtilities::Vector4f ReadVector4(const Element& element)
	{
		const auto values = element.get_array().value();
		if (values.size() != 4)
		{
			throw std::runtime_error("color/vector must contain 4 numbers");
		}
		return {
			static_cast<float>(values.at(0).get_double().value()),
			static_cast<float>(values.at(1).get_double().value()),
			static_cast<float>(values.at(2).get_double().value()),
			static_cast<float>(values.at(3).get_double().value())};
	}

	std::vector<std::string> ReadStrings(const Element& element)
	{
		std::vector<std::string> result;
		for (const auto value : element.get_array().value())
		{
			result.emplace_back(std::string_view(value.get_string().value()));
		}
		return result;
	}

	BaseComponentData ReadBaseComponent(const Object& source, UnrealComponentType type)
	{
		BaseComponentData result;
		result.Name = ReadString(source, "Name");
		result.TypeID = type;
		result.Parent = ReadOptionalString(source, "Parent");
		result.Tags = ReadStrings(source["Tags"]);
		result.Transform = ReadMatrix(source["Transform"]);
		return result;
	}

	ImportedMaterial ReadMaterial(const Object& source)
	{
		ImportedMaterial result;
		result.Name = ReadString(source, "Name");
		result.Parent = ReadOptionalString(source, "Parent");

		for (const auto value : source["Parameters"].get_array().value())
		{
			const auto sourceParameter = value.get_object().value();
			ImportedMaterialParameter parameter;
			parameter.Name = ReadString(sourceParameter, "Name");
			parameter.Type = static_cast<MaterialType>(sourceParameter["Type"].get_int64().value());

			switch (parameter.Type)
			{
			case MaterialType::Scalar:
				parameter.Value = ReadNumber(sourceParameter, "Value");
				break;
			case MaterialType::Vector3f:
				parameter.Value = ReadVector4(sourceParameter["Value"]);
				break;
			case MaterialType::Texture:
			{
				const auto texture = sourceParameter["Value"].get_object().value();
				parameter.Value = TextureValue{ReadString(texture, "Name"), ReadString(texture, "Path")};
				break;
			}
			default:
				// Match the supported material set. Additional exported parameter
				// types can be implemented by the owning team when their runtime semantics are known.
				continue;
			}

			// Bug fix: the first integration inserted a default parameter and then inserted
			// the populated copy, producing two entries for every supported parameter.
			result.Parameters.push_back(std::move(parameter));
		}
		return result;
	}

	ImportedMeshComponent ReadMeshComponent(
		const Object& source,
		BaseComponentData base)
	{
		ImportedMeshComponent result;
		static_cast<BaseComponentData&>(result) = std::move(base);
		result.Mesh = ReadString(source, "Mesh");

		// ContentPath is consumed by the adapter/runtime asset resolver. Mesh keeps the
		// exported display identity separately, so neither source value is lost.
		result.ContentPath = ReadString(source, "ContentPath");
		for (const auto material : source["Materials"].get_array().value())
		{
			// Bug fix: ReadMaterial returns the one material already populated above.
			// The first integration appended that same material a second time.
			result.Materials.push_back(ReadMaterial(material.get_object().value()));
		}
		return result;
	}

	void ReadLight(const Object& source, ImportedLightComponent& result)
	{
		// Bug fix: the first integration read element zero into every channel.
		result.Color = ReadVector4(source["Color"]);
		result.Intensity = ReadNumber(source, "Intensity");
	}
}

UnrealImportResult UnrealSceneImporter::ImportScene(const std::filesystem::path& jsonPath) const
{
	UnrealImportResult result;
	try
	{
		const auto json = simdjson::padded_string::load(jsonPath.string()).value();
		simdjson::dom::parser parser;
		const auto root = parser.parse(json).get_object().value();
		UnrealSceneData scene;

		for (const auto actorValue : root["Actors"].get_array().value())
		{
			const auto sourceActor = actorValue.get_object().value();
			UnrealActorData actor;
			actor.Name = ReadString(sourceActor, "Name");
			actor.Archetype = ReadString(sourceActor, "Archetype");
			actor.Tags = ReadStrings(sourceActor["Tags"]);
			actor.Transform = ReadMatrix(sourceActor["Transform"]);

			for (const auto componentValue : sourceActor["Components"].get_array().value())
			{
				const auto source = componentValue.get_object().value();
				const auto rawType = source["TypeID"].get_int64().value();
				if (rawType < static_cast<int64_t>(UnrealComponentType::Custom) ||
					rawType > static_cast<int64_t>(UnrealComponentType::SpringArm))
				{
					// Bug fix: the first integration silently discarded future/invalid TypeIDs,
					// which could make a partially loaded scene appear successful.
					throw std::runtime_error(actor.Name + ": unknown component TypeID " + std::to_string(rawType));
				}

				const auto type = static_cast<UnrealComponentType>(rawType);
				auto base = ReadBaseComponent(source, type);
				switch (type)
				{
				case UnrealComponentType::Custom:
					actor.Components.push_back(std::move(base));
					break;
				case UnrealComponentType::SceneComponent:
					// The adapter turns tagged scene components into cameras and the remaining
					// records into runtime SceneComponents, so they must not be skipped here.
					actor.Components.push_back(std::move(base));
					break;
				case UnrealComponentType::StaticMesh:
				case UnrealComponentType::SkeletalMesh:
					// Bug fix: preserve the original TypeID. The adapter selects StaticMeshData
					// or SkeletalMeshData after parsing instead of treating both as static meshes.
					actor.Components.push_back(ReadMeshComponent(source, std::move(base)));
					break;
				case UnrealComponentType::PointLight:
				{
					ImportedPointLightComponent light;
					static_cast<BaseComponentData&>(light) = std::move(base);
					// Bug fix: keep PointLight as PointLight; the first integration marked it SpotLight.
					ReadLight(source, light);
					light.FalloffExponent = ReadNumber(source, "FalloffExponent");
					light.AttenuationRadius = ReadNumber(source, "AttenuationRadius");
					actor.Components.push_back(std::move(light));
					break;
				}
				case UnrealComponentType::SpotLight:
				{
					ImportedSpotLightComponent light;
					static_cast<BaseComponentData&>(light) = std::move(base);
					ReadLight(source, light);
					light.FalloffExponent = ReadNumber(source, "FalloffExponent");
					light.AttenuationRadius = ReadNumber(source, "AttenuationRadius");
					light.InnerConeAngle = ReadNumber(source, "InnerConeAngle");
					light.OuterConeAngle = ReadNumber(source, "OuterConeAngle");
					actor.Components.push_back(std::move(light));
					break;
				}
				case UnrealComponentType::DirectionalLight:
				{
					ImportedLightComponent light;
					static_cast<BaseComponentData&>(light) = std::move(base);
					ReadLight(source, light);
					actor.Components.push_back(std::move(light));
					break;
				}
				case UnrealComponentType::Box:
				{
					ImportedBoxComponent box;
					// Bug fix: retain the box's own TypeID and all common component metadata.
					static_cast<BaseComponentData&>(box) = std::move(base);
					box.BoundsMaxima = ReadVector3(source["Bounds"]);
					actor.Components.push_back(std::move(box));
					break;
				}
				case UnrealComponentType::Sphere:
				{
					ImportedSphereComponent sphere;
					// Bug fix: retain the sphere's own TypeID and all common component metadata.
					static_cast<BaseComponentData&>(sphere) = std::move(base);
					sphere.Radius = ReadNumber(source, "Radius");
					actor.Components.push_back(std::move(sphere));
					break;
				}
				case UnrealComponentType::Capsule:
				{
					ImportedCapsuleComponent capsule;
					// Bug fix: the first integration kept only radius/height and lost name,
					// tags, parent and transform.
					static_cast<BaseComponentData&>(capsule) = std::move(base);
					capsule.Radius = ReadNumber(source, "Radius");
					capsule.HalfHeight = ReadNumber(source, "HalfHeight");
					actor.Components.push_back(std::move(capsule));
					break;
				}
				case UnrealComponentType::SpringArm:
				{
					ImportedSpringArmComponent springArm;
					// Bug fix: retain the spring arm's TypeID and exported settings.
					static_cast<BaseComponentData&>(springArm) = std::move(base);
					springArm.SocketOffset = ReadVector3Object(source["SocketOffset"].get_object().value());
					springArm.ArmLength = ReadNumber(source, "ArmLength");
					actor.Components.push_back(std::move(springArm));
					break;
				}
				}
			}
			scene.Actors.push_back(std::move(actor));
		}

		// UnrealSceneAdapter performs engine-facing conversion. ComponentRegistry then
		// resolves assets and builds a candidate World before the active World is replaced.
		result.Data = std::move(scene);
	}
	catch (const std::exception& error)
	{
		result.Diagnostics.push_back({jsonPath.string(), error.what()});
	}
	return result;
}
