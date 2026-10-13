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

struct CustomComponentProperties
{
	std::unordered_map<std::string, int64_t> Integers;
	std::unordered_map<std::string, float> Floats;
	std::unordered_map<std::string, bool> Booleans;
	std::unordered_map<std::string, std::string> Strings;
	std::unordered_map<std::string, CommonUtilities::Vector3f> Vectors;
	std::unordered_map<std::string, CustomTransformData> Transforms;

	bool Contains(const std::string& aName) const
	{
		return Integers.contains(aName) || Floats.contains(aName) || Booleans.contains(aName) || Strings.contains(aName) ||
		       Vectors.contains(aName) || Transforms.contains(aName);
	}

	template <class T> T Get(const std::string& aName, T aDefaultValue) const
	{
		const auto& properties = GetMap<T>();
		const auto property = properties.find(aName);
		if (property != properties.end())
		{
			return property->second;
		}
		if constexpr (std::is_same_v<T, float>)
		{
			const auto integer = Integers.find(aName);
			if (integer != Integers.end())
			{
				return static_cast<float>(integer->second);
			}
		}
		if (Contains(aName))
		{
			throw std::invalid_argument("Wrong type for custom property: " + aName);
		}
		return aDefaultValue;
	}

private:
	template <class T> const std::unordered_map<std::string, T>& GetMap() const
	{
		if constexpr (std::is_same_v<T, int64_t>)
		{
			return Integers;
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			return Floats;
		}
		else if constexpr (std::is_same_v<T, bool>)
		{
			return Booleans;
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			return Strings;
		}
		else if constexpr (std::is_same_v<T, CommonUtilities::Vector3f>)
		{
			return Vectors;
		}
		else
		{
			static_assert(std::is_same_v<T, CustomTransformData>);
			return Transforms;
		}
	}
};
