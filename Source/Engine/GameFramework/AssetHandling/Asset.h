#pragma once

#include <filesystem>
#include <string_view>

class AssetRegistry;

class Asset
{
    friend class AssetRegistry;

public:
    virtual ~Asset();

    std::string_view GetName() const { return myName; };

protected:
    virtual bool Load(const std::filesystem::path& aPath, AssetRegistry& aRegistry) = 0;

private:
    std::string myName;
};

