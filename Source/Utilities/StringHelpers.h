/**
 * String Helpers
 * by Daniel Borgshammar @ TGA
 */
namespace string_helpers
{
	/**
	 * \brief inspects a string to check if it's a valid UTF-8 encoded string.
	 */
	inline bool is_valid_utf8(const std::string& aString)
	{
		if (aString.empty())
			return true;

		auto bytes = reinterpret_cast<const unsigned char*>(aString.data());
		unsigned int cp;
		int num;

		while (*bytes != 0x00)
		{
			if ((*bytes & 0x80) == 0x00)
			{
				// U+0000 to U+007F 
				cp = (*bytes & 0x7F);
				num = 1;
			}
			else if ((*bytes & 0xE0) == 0xC0)
			{
				// U+0080 to U+07FF 
				cp = (*bytes & 0x1F);
				num = 2;
			}
			else if ((*bytes & 0xF0) == 0xE0)
			{
				// U+0800 to U+FFFF 
				cp = (*bytes & 0x0F);
				num = 3;
			}
			else if ((*bytes & 0xF8) == 0xF0)
			{
				// U+10000 to U+10FFFF 
				cp = (*bytes & 0x07);
				num = 4;
			}
			else
				return false;

			bytes += 1;
			for (int i = 1; i < num; ++i)
			{
				if ((*bytes & 0xC0) != 0x80)
					return false;
				cp = (cp << 6) | (*bytes & 0x3F);
				bytes += 1;
			}

			if ((cp > 0x10FFFF) ||
				((cp >= 0xD800) && (cp <= 0xDFFF)) ||
				((cp <= 0x007F) && (num != 1)) ||
				((cp >= 0x0080) && (cp <= 0x07FF) && (num != 2)) ||
				((cp >= 0x0800) && (cp <= 0xFFFF) && (num != 3)) ||
				((cp >= 0x10000) && (cp <= 0x1FFFFF) && (num != 4)))
				return false;
		}

		return true;
	}

	/**
	 * \brief Converts a ACP (Application Code Page) string to UTF-8.
	 */
	inline std::string acp_to_utf8(const std::string& aString)
	{
		const int sLength = static_cast<int>(aString.length());
		if(sLength == 0)
			return {};

		int len = MultiByteToWideChar(CP_ACP, 0, aString.c_str(), sLength, 0, 0);
		std::wstring wideString(len, L'\0');
		MultiByteToWideChar(CP_ACP, 0, aString.c_str(), -1, wideString.data(), len);

		len = WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), sLength, 0, 0, 0, 0);
		std::string result(len, L'\0');
		WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), -1, result.data(), len, 0, 0);
		return result;
	}

	/**
	 * \brief Converts a UTF-8 string to ACP (Application Code Page).
	 */
	inline std::string utf8_to_acp(const std::string& aString)
	{
		const int sLength = static_cast<int>(aString.length());
		if(sLength == 0)
			return {};

		int len = MultiByteToWideChar(CP_UTF8, 0, aString.c_str(), sLength, 0, 0);
		std::wstring wideString(len, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, aString.c_str(), -1, wideString.data(), len);

		len = WideCharToMultiByte(CP_ACP, 0, wideString.c_str(), sLength, 0, 0, 0, 0);
		std::string result(len, L'\0');
		WideCharToMultiByte(CP_ACP, 0, wideString.c_str(), -1, result.data(), len, 0, 0);
		return result;
	}

	/**
	 * \brief Converts a Wide String (UTF-16LE) to UTF-8.
	 */
	inline std::string wide_to_utf8(const std::wstring& aString)
	{
		const int sLength = static_cast<int>(aString.length());
		if(sLength == 0)
			return {};

		int len = WideCharToMultiByte(CP_UTF8, 0, aString.c_str(), sLength, 0, 0, 0, 0);
		std::string result(len, L'\0');
		WideCharToMultiByte(CP_UTF8, 0, aString.c_str(), -1, result.data(), len, 0, 0);
		return result;
	}

	/**
	 * \brief Converts a UTF-8 String to Wide-String (UTF-16LE).
	 */
	inline std::wstring utf8_to_wide(const std::string& aString)
	{
		const int sLength = static_cast<int>(aString.length());
		if(sLength == 0)
			return {};
		const int len = MultiByteToWideChar(CP_UTF8, 0, aString.c_str(), sLength, 0, 0);
		std::wstring result(len, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, aString.c_str(), -1, result.data(), len);
		return result;
	}

	/**
	 * \brief Converts a Wide String (UTF-16LE) to ACP (Application Code Page) string.
	 */
	inline std::string wide_to_acp(const std::wstring& aString)
	{
		const int sLength = static_cast<int>(aString.length());
		if(sLength == 0)
			return {};
		const int len = WideCharToMultiByte(CP_ACP, 0, aString.c_str(), sLength, 0, 0, 0, 0);
		std::string result(len, L'\0');
		WideCharToMultiByte(CP_ACP, 0, aString.c_str(), -1, result.data(), len, 0, 0);
		return result;
	}

	/**
	 * \brief Converts a ACP (Application Code Page) string to Wide String (UTF-16LE).
	 */
	inline std::wstring acp_to_wide(const std::string& aString)
	{
		const int sLength = static_cast<int>(aString.length());
		if(sLength == 0)
			return {};
		const int len = MultiByteToWideChar(CP_ACP, 0, aString.c_str(), sLength, 0, 0);
		std::wstring result(len, L'\0');
		MultiByteToWideChar(CP_ACP, 0, aString.c_str(), -1, result.data(), len);
		return result;
	}

	/**
	 * \brief Cuts a string into several pieces based on the provided delimiter.
	 */
	inline std::vector<std::string> explode_string(const std::string& aString, const std::string& aDelimiter)
	{
		std::vector<std::string> result;

		size_t start = 0ULL;
		size_t end = aString.find(aDelimiter);

		while(end != std::string::npos)
		{
			result.push_back(aString.substr(start, end - start));
			start = end + aDelimiter.length();
			end = aString.find(aDelimiter, start);
		}

		result.push_back(aString.substr(start));

		return result;
	}

	/**
	 * \brief Cuts a string into several pieces based on the provided delimiter and also trims each string.
	 */
	inline std::vector<std::string> explode_and_trim_string(const std::string& aString, const std::string& aDelimiter)
	{
		std::vector<std::string> result;

		size_t start = 0ULL;
		size_t end = aString.find(aDelimiter);

		while(end != std::string::npos)
		{
			std::string substring = aString.substr(start, end - start);
			substring.erase(substring.find_last_not_of(L' ') + 1);
			substring.erase(0, substring.find_first_not_of(L' '));
			if(!substring.empty())
			{
				result.push_back(substring);
			}

			start = end + aDelimiter.length();
			end = aString.find(aDelimiter, start);
		}

		std::string substring = aString.substr(start);
		substring.erase(substring.find_last_not_of(L' ') + 1);
		substring.erase(0, substring.find_first_not_of(L' '));
		if(!substring.empty())
		{
			result.push_back(substring);
		}

		return result;
	}

	/**
	 * @brief Left-Trim the specified string in-place for occurrences of whitespace characters (space, tab, newline etc).
	 * @param s The string to trim.
	*/
	inline void ltrim_in_place(std::string& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return !std::isspace(ch);
			}));
	}

	/**
	 * @brief Right-Trim the specified string in-place for occurrences of whitespace characters (space, tab, newline etc).
	 * @param s The string to trim.
	*/
	inline void rtrim_in_place(std::string& s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
			}).base(), s.end());
	}

	/**
	 * @brief Trim the specified string in-place for occurrences of whitespace characters (space, tab, newline etc).
	*/
	inline void trim_in_place(std::string& s) {
		ltrim_in_place(s);
		rtrim_in_place(s);
	}
	/**
	 * @brief Right-Trim the specified string in-place for occurrences of the specified character.
	*/
	inline void rtrim_in_place(std::string& s, const char c)	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [c](char ch) { return ch != c; }).base(), s.end());
	}

	/**
	 * @brief Left-Trim the specified string in-place for occurrences of the specified character.
	*/
	inline void ltrim_in_place(std::string& s, const char c) {
		s.erase(s.begin(), std::ranges::find_if(s, [c](char ch) { return ch != c; }));
	}

	/**
	 * @brief Trim the specified string in-place for occurrences of the specified character.
	*/
	inline void trim_in_place(std::string& s, const char c) {
		ltrim_in_place(s, c);
		rtrim_in_place(s, c);
	}

	/**
	 * @brief In-place conversion of a string to lower case.
	 * NOTE: Only works reliably on ACP strings!
	*/
	inline void to_lower_in_place(std::string& s)
	{
		std::ranges::transform(s, s.begin(),
		                       [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
	}

	/**
	 * @brief In-place conversion of a string to upper case.
	 * NOTE: Only works reliably on ACP strings!
	*/
	inline void to_upper_in_place(std::string& s)
	{
		std::ranges::transform(s, s.begin(),
		                       [](unsigned char c) { return static_cast<unsigned char>(std::toupper(c)); });

	}
}

/**
 * String Helpers
 * by Daniel Borgshammar @ TGA
 */
namespace str = string_helpers;