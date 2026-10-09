#pragma once
#include <string>
struct LegacyAtlasImage;
namespace BuiltinResources {
bool buildTexture(const std::string& logicalName, LegacyAtlasImage& out);
}
