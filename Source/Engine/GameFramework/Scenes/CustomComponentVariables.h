#pragma once
#include "Vector3.hpp"
#include "Vector4.hpp"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

// Authored export coordinates; Rotation is quaternion x, y, z, w.
struct CustomTransformData
{
	CommonUtilities::Vector3f Translation;
	CommonUtilities::Vector4f Rotation{0, 0, 0, 1};
	CommonUtilities::Vector3f Scale{1, 1, 1};
};

// Mirrors the six variable maps in Unreal.
struct CustomComponentVariables
{
	std::unordered_map<std::string, int64_t> IntVariables;
	std::unordered_map<std::string, float> FloatVariables;
	std::unordered_map<std::string, bool> BoolVariables;
	std::unordered_map<std::string, std::string> StringVariables;
	std::unordered_map<std::string, CommonUtilities::Vector3f> VectorVariables;
	std::unordered_map<std::string, CustomTransformData> TransformVariables;

	bool Contains(const std::string& aVariableName) const
	{
		return IntVariables.contains(aVariableName) || FloatVariables.contains(aVariableName) || BoolVariables.contains(aVariableName) || StringVariables.contains(aVariableName) ||
		       VectorVariables.contains(aVariableName) || TransformVariables.contains(aVariableName);
	}

	// Returns the default only when the name is missing.
	// Throws if the variable exists with a different type. No type conversion.
	template <class T> T GetOrDefault(const std::string& aVariableName, T aDefaultValue) const
	{
		const auto& variables = GetMap<T>();
		const auto variable = variables.find(aVariableName);
		if (variable != variables.end())
		{
			return variable->second;
		}
		if (Contains(aVariableName))
		{
			throw std::invalid_argument("Wrong type for custom variable: " + aVariableName);
		}
		return aDefaultValue;
	}

private:
	template <class T>
	const std::unordered_map<std::string, T>& GetMap() const
	{
		if constexpr (std::is_same_v<T, int64_t>)
		{
			return IntVariables;
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			return FloatVariables;
		}
		else if constexpr (std::is_same_v<T, bool>)
		{
			return BoolVariables;
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			return StringVariables;
		}
		else if constexpr (std::is_same_v<T, CommonUtilities::Vector3f>)
		{
			return VectorVariables;
		}
		else
		{
			static_assert(std::is_same_v<T, CustomTransformData>);
			return TransformVariables;
		}
	}
};
