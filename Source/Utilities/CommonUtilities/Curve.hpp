#pragma once

#include "Maths.hpp"

#include <assert.h>
#include <unordered_map>

namespace CommonUtilities
{
	template<typename T>
	class Curve
	{
	public:
		bool AddKey(float aPosition, const T& aValue)
		{
			assert(aPosition >= 0.0f && aPosition <= 1.0f);
			const unsigned realValue = static_cast<unsigned>(aPosition * ourPrecision);

			bool posFound = false;
			for (unsigned pos : myKeyPositions)
			{
				if (pos == realValue)
				{
					posFound = true;
					break;
				}
			}

			if (!posFound)
				myKeyPositions.emplace_back(realValue);

			myKeys.emplace(realValue, aValue);

			return true;
		}

		void Clear()
		{
			myKeyPositions.clear();
			myKeys.clear();
		}

		T Get(float aPosition) const
		{
			const unsigned realValue = static_cast<unsigned>(aPosition * ourPrecision);

			T startValue, endValue;

			for (auto it = myKeyPositions.begin(); it != myKeyPositions.end(); ++it)
			{
				if (*it == realValue)
				{
					return myKeys.at(*it);
				}

				if (*it < realValue)
				{
					if (it + 1 != myKeyPositions.end())
					{
						if (*(it + 1) > realValue)
						{
							const float startPos = static_cast<float>(*it) / ourPrecision;
							const float endPos = static_cast<float>(*(it + 1)) / ourPrecision;

							startValue = myKeys.at(*it);
							endValue = myKeys.at(*(it + 1));

							const float a = Maths::Remap(aPosition, startPos, endPos, 0.0f, 1.0f);

							return Math::Lerp(startValue, endValue, a);
						}
					}
					else
					{
						return myKeys.at(*it);
					}
				}
				else if (it == myKeyPositions.begin() && realValue < *it)
				{
					return myKeys.at(*it);
				}
			}

			return T();
		}

	private:
		float ourPrecision = 100000.0f;

		std::vector<unsigned> myKeyPositions;
		std::unordered_map<unsigned, T> myKeys;
	};
}
