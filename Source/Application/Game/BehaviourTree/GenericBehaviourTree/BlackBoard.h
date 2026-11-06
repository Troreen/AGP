#pragma once
#include <unordered_map>
#include <any>
#include <string>
#include <cassert>

class BlackBoard
{
public:
	BlackBoard() = default;
	~BlackBoard() = default;
	BlackBoard(const BlackBoard&) = delete;
	BlackBoard& operator=(const BlackBoard&) = delete;
	BlackBoard(BlackBoard&&) = delete;
	BlackBoard& operator=(const BlackBoard&&) = delete;

	template <typename T>
	void Set(const std::string& aKey, T&& someData);

	template <typename T>
	T& Get(const std::string& aKey);

	template <typename T>
	T& SetGet(const std::string& aKey, T&& someData);

private:
	std::unordered_map<std::string, std::any> myData;
};

template<typename T>
inline void BlackBoard::Set(const std::string& aKey, T&& someData)
{
	myData.insert_or_assign(aKey, std::forward<T>(someData));
}

template<typename T>
inline T& BlackBoard::Get(const std::string& aKey)
{
	assert(myData.contains(aKey) == true);

	return std::any_cast<T&>(myData.at(aKey));
}

template<typename T>
inline T& BlackBoard::SetGet(const std::string& aKey, T&& someData)
{
	myData.insert_or_assign(aKey, std::forward<T>(someData));

	return std::any_cast<T&>(myData.at(aKey));
}
