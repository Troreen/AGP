#pragma once

namespace CommonUtilities
{
	template <class T>
	struct Rectangle
	{
		T Top{};
		T Left{};
		T Bottom{};
		T Right{};
	};

	using Rectanglef = Rectangle<float>;
}
