#pragma once

#include "../SimdJson/simdjson.h"

#include <CommonUtilities/CommonUtilities.h>
#include <CommonUtilities/Curve.hpp>
#include <filesystem>
#include <optional>

using JsonObject = simdjson::dom::object;
using JsonResult = simdjson::simdjson_result<simdjson::dom::element>;
using JsonElement = simdjson::dom::element;
using JsonArray = simdjson::dom::array;

namespace TMJson
{
	template<class T>
	static inline bool GetElement_s(const JsonElement& aParent, std::string_view aElementName, T& outElement)
	{
		return false;
	}

	template<>
	inline bool GetElement_s<JsonElement>(const JsonElement& aParent, std::string_view aElementName, JsonElement& outElement)
	{
		JsonResult element = aParent.at_key(aElementName);
		if (element.error() == simdjson::NO_SUCH_FIELD)
		{
			return false;
		}

		outElement = element.value();

		return true;
	}

	template<>
	inline bool GetElement_s<JsonResult>(const JsonElement& aParent, std::string_view aElementName, JsonResult& outElement)
	{
		outElement = aParent.at_key(aElementName);
		return true;
	}

	template<>
	inline bool GetElement_s<bool>(const JsonElement& aParent, std::string_view aElementName, bool& outElement)
	{
		JsonResult element = aParent.at_key(aElementName);
		if (element.error() == simdjson::NO_SUCH_FIELD ||
			!element.is_bool())
		{
			return false;
		}

		outElement = element.get_bool();

		return true;
	}

	template<>
	inline bool GetElement_s<float>(const JsonElement& aParent, std::string_view aElementName, float& outElement)
	{
		JsonResult element = aParent.at_key(aElementName);
		if (element.error() == simdjson::NO_SUCH_FIELD ||
			!element.is_number())
		{
			return false;
		}

		outElement = static_cast<float>(element.get_double());

		return true;
	}

	template<>
	inline bool GetElement_s<int>(const JsonElement& aParent, std::string_view aElementName, int& outElement)
	{
		JsonResult element = aParent.at_key(aElementName);
		if (element.error() == simdjson::NO_SUCH_FIELD ||
			!element.is_number())
		{
			return false;
		}

		outElement = static_cast<int>(element.get_int64());

		return true;
	}

	template<>
	inline bool GetElement_s<unsigned>(const JsonElement& aParent, std::string_view aElementName, unsigned& outElement)
	{
		JsonResult element = aParent.at_key(aElementName);
		if (element.error() == simdjson::NO_SUCH_FIELD ||
			!element.is_number())
		{
			return false;
		}

		outElement = static_cast<int>(element.get_uint64());

		return true;
	}

	template<>
	inline bool GetElement_s<std::string>(const JsonElement& aParent, std::string_view aElementName, std::string& outElement)
	{
		JsonResult element = aParent.at_key(aElementName);
		if (element.error() == simdjson::NO_SUCH_FIELD ||
			!element.is_string())
		{
			return false;
		}

		outElement = element.get_string().value();

		return true;
	}

	template<>
	inline bool GetElement_s<std::string_view>(const JsonElement& aParent, std::string_view aElementName, std::string_view& outElement)
	{
		JsonResult element = aParent.at_key(aElementName);
		if (element.error() == simdjson::NO_SUCH_FIELD ||
			!element.is_string())
		{
			return false;
		}

		outElement = element.get_string().value();

		return true;
	}

	template<>
	inline bool GetElement_s<CU::Vector2f>(const JsonElement& aParent, std::string_view aElementName, CU::Vector2f& outElement)
	{
		JsonResult element = aParent.at_key(aElementName);
		if (element.error() == simdjson::NO_SUCH_FIELD ||
			!element.is_array() || element.get_array().size() != 2)
		{
			return false;
		}

		JsonArray array = element.get_array().value();
		if (!array.at(0).is_number() ||
			!array.at(1).is_number())
		{
			return false;
		}

		outElement.x = static_cast<float>(array.at(0).get_double());
		outElement.y = static_cast<float>(array.at(1).get_double());

		return true;
	}

	template<>
	inline bool GetElement_s<CU::Vector3f>(const JsonElement& aParent, std::string_view aElementName, CU::Vector3f& outElement)
	{
		JsonResult element = aParent.at_key(aElementName);
		if (element.error() == simdjson::NO_SUCH_FIELD ||
			!element.is_array() || element.get_array().size() != 3)
		{
			return false;
		}

		JsonArray array = element.get_array().value();
		if (!array.at(0).is_number() ||
			!array.at(1).is_number() ||
			!array.at(2).is_number())
		{
			return false;
		}

		outElement.x = static_cast<float>(array.at(0).get_double());
		outElement.y = static_cast<float>(array.at(1).get_double());
		outElement.z = static_cast<float>(array.at(2).get_double());

		return true;
	}

	template<>
	inline bool GetElement_s<CU::Vector4f>(const JsonElement& aParent, std::string_view aElementName, CU::Vector4f& outElement)
	{
		JsonResult element = aParent.at_key(aElementName);
		if (element.error() == simdjson::NO_SUCH_FIELD ||
			!element.is_array() || element.get_array().size() != 4)
		{
			return false;
		}

		JsonArray array = element.get_array().value();
		if (!array.at(0).is_number() ||
			!array.at(1).is_number() ||
			!array.at(2).is_number() ||
			!array.at(3).is_number())
		{
			return false;
		}

		outElement.x = static_cast<float>(array.at(0).get_double());
		outElement.y = static_cast<float>(array.at(1).get_double());
		outElement.z = static_cast<float>(array.at(2).get_double());
		outElement.w = static_cast<float>(array.at(3).get_double());

		return true;
	}

	template<class T>
	inline bool GetCurveElement_s(const JsonElement& aParent, std::string_view aElementName, CU::Curve<T>& outCurve)
	{
		JsonResult keyValueElements = aParent.at_key(aElementName);
		if (keyValueElements.error() == simdjson::NO_SUCH_FIELD)
		{
			return false;
		}

		outCurve.Clear();

		for (const JsonElement& element : keyValueElements)
		{
			const JsonResult& key = element.at_key("Key");
			if (key.error() == simdjson::NO_SUCH_FIELD)
			{
				continue;
			}
			if (!key.is_number())
			{
				continue;
			}

			T value{};
			if (!GetElement_s<T>(element, "Value", value))
			{
				continue;
			}

			const float keyValue = static_cast<float>(key.get_double());
			outCurve.AddKey(keyValue, value);
		}

		return true;
	}
}
