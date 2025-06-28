#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace MR::GPU {
using PlaceHoldersMap = std::unordered_map<std::string, std::string>;
using MacroDefinitions = std::unordered_set<std::string>;
std::string preprocessWGSLFile(const std::filesystem::path& filePath,
                               const PlaceHoldersMap& placeholders,
                               const MacroDefinitions& macros);

std::string preprocessWGSLString(const std::string& shaderText,
                                 const PlaceHoldersMap& placeholders,
                                 const MacroDefinitions& macros);
}
