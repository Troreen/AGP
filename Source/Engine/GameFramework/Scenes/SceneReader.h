#pragma once
#include "GameFramework/Scenes/SceneData.h"
#include <cmath>
#include <limits>
#include <stdexcept>

// A small typed property reader. No reflection, reference fixups or hidden state.
class SceneReader
{
public:
	SceneReader(const PropertyMap& fields, const AssetLibrary& assets, CommonUtilities::Vector2u size)
	    : myFields(fields), myAssets(assets), mySize(size)
	{
	}

	bool Has(const std::string& field) const
	{
		return myFields.contains(field);
	}

	template <class T> T Get(const std::string& field, T fallback) const
	{
		const auto it = myFields.find(field);
		if (it == myFields.end())
		{
			return fallback;
		}
		if (const auto* value = std::get_if<T>(&it->second))
		{
			return *value;
		}
		Error(field, "Wrong property type");
	}

	float OptionalFloat(const std::string& field, float fallback) const
	{
		const auto it = myFields.find(field);
		if (it == myFields.end())
		{
			return fallback;
		}
		double value;
		if (const auto* real = std::get_if<double>(&it->second))
		{
			value = *real;
		}
		else if (const auto* integer = std::get_if<int64_t>(&it->second))
		{
			value = double(*integer);
		}
		else
		{
			Error(field, "Expected a number");
		}
		if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max())
		{
			Error(field, "Number must be finite and fit a float");
		}
		return float(value);
	}

	bool OptionalBool(const std::string& field, bool fallback) const
	{
		return Get(field, fallback);
	}

	std::string OptionalString(const std::string& field, std::string fallback = {}) const
	{
		return Get(field, fallback);
	}

	CommonUtilities::Vector3f OptionalVector3(const std::string& field, CommonUtilities::Vector3f fallback) const
	{
		const auto value = Get(field, fallback);
		if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z))
		{
			Error(field, "Vector must be finite");
		}
		return value;
	}

	std::vector<AssetId> OptionalAssets(const std::string& field) const
	{
		return Get(field, std::vector<AssetId>{});
	}

	AssetId RequiredAsset(const std::string& field) const
	{
		const auto asset = Get(field, AssetId{});
		if (asset.Value.empty())
		{
			Error(field, "Asset is required");
		}
		return asset;
	}

	const AssetLibrary& GetAssets() const
	{
		return myAssets;
	}

	CommonUtilities::Vector2u GetClientSize() const
	{
		return mySize;
	}

	[[noreturn]] void Error(const std::string& field, const std::string& message) const
	{
		throw std::runtime_error(field + ": " + message);
	}

private:
	const PropertyMap& myFields;
	const AssetLibrary& myAssets;
	CommonUtilities::Vector2u mySize;
};
