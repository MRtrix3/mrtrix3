#include "wgslprocessing.h"
#include "utils.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std::string_literals;


namespace MR::GPU {

namespace {
std::string replacePlaceholders(const std::string& line,
                                const std::unordered_map<std::string, std::string>& substitutions) {
    const std::regex placeholder_regex(R"(\{\{([^{}]+)\}\})");
    std::string result;
    size_t last_pos = 0;

    std::sregex_iterator it(line.begin(), line.end(), placeholder_regex);
    const std::sregex_iterator end;

    for (; it != end; ++it) {
        const std::smatch& match = *it;
        const size_t start = match.position();
        const size_t length = match.length();
        const std::string key = match[1].str();

        result += line.substr(last_pos, start - last_pos);

        auto sub_it = substitutions.find(key);
        if (sub_it != substitutions.end()) {
            result += sub_it->second;
        } else {
            result += match.str(); // Leave unknown placeholders intact
        }

        last_pos = start + length;
    }

    result += line.substr(last_pos);

    return result;
}

std::string_view trimLeadingWhitespace(std::string_view s) {
    size_t const start = s.find_first_not_of(" \t");
    return (start == std::string::npos) ? "" : s.substr(start);
}

std::string preprocessRecursive(
    const std::filesystem::path& currentPathContext,
    std::unordered_set<std::string>& visitedFilePaths,
    const MacroDefinitions& definedMacros,
    const std::string* initialCode = nullptr
)
{
    const std::filesystem::path normalizedPathKey = currentPathContext.lexically_normal();
    const std::string pathKeyStr = normalizedPathKey.string();

    // Detect cycles
    if (visitedFilePaths.count(pathKeyStr) > 0) {
        throw std::runtime_error("Detected recursive include of " + pathKeyStr);
    }
    visitedFilePaths.insert(pathKeyStr);

    std::string code;
    if (initialCode != nullptr) {
        code = *initialCode;
    }
    else {
        if (!std::filesystem::exists(normalizedPathKey)) {
            throw std::runtime_error("File not found: " + pathKeyStr);
        }
        code = Utils::readFile(normalizedPathKey);
    }

    std::stringstream inputStream(code);
    std::stringstream outputStream;
    // Tracks nesting of #ifdef/#else blocks
    std::vector<bool> conditionStack;
    std::string line;

    while (std::getline(inputStream, line)) {
        const std::string_view trimmedLine = trimLeadingWhitespace(line);

        if (trimmedLine.rfind("#ifdef", 0) == 0) {
            std::istringstream iss((std::string(trimmedLine)));
            std::string directive;
            std::string macro;
            iss >> directive >> macro;
            const bool parentActive = std::all_of(conditionStack.begin(), conditionStack.end(), [](bool b){ return b; });
            const bool isDefined = parentActive && ((definedMacros.count(macro)) != 0U);
            conditionStack.push_back(isDefined);
            continue;
        }
        else if (trimmedLine.rfind("#else", 0) == 0) {
            if (conditionStack.empty()) {
                throw std::runtime_error("Unmatched #else directive in " + pathKeyStr);
            }
            const bool previousCondition = conditionStack.back();
            conditionStack.pop_back();
            const bool parentActive = std::all_of(conditionStack.begin(), conditionStack.end(), [](bool b){ return b; });
            const bool newCondition = parentActive ? !previousCondition : false;
            conditionStack.push_back(newCondition);
            continue;
        }
        else if (trimmedLine.rfind("#endif", 0) == 0) {
            if (conditionStack.empty()) {
                throw std::runtime_error("Unmatched #endif directive in " + pathKeyStr);
            }
            conditionStack.pop_back();
            continue;
        }

        const bool currentActive = std::all_of(conditionStack.begin(), conditionStack.end(), [](bool b){ return b; });
        if (currentActive && trimmedLine.rfind("#include", 0) == 0) {
            const auto startQuote = trimmedLine.find_first_of("\"<");
            const auto endQuote   = trimmedLine.find_last_of("\">");
            if (startQuote != std::string::npos && endQuote != std::string::npos && endQuote > startQuote) {
                const std::string_view includePathStrView = trimmedLine.substr(startQuote + 1, endQuote - (startQuote + 1));
                const std::filesystem::path includeDirectivePath(std::string{includePathStrView});

                std::filesystem::path fullPathToInclude;
                const std::filesystem::path baseDir = normalizedPathKey.parent_path();

                if (includeDirectivePath.is_absolute()) {
                    fullPathToInclude = includeDirectivePath;
                } else {
                    fullPathToInclude = baseDir / includeDirectivePath;
                }

                // Recursively process the included file content. initialCode is nullptr, so it will be read.
                const std::string includedCode = preprocessRecursive(fullPathToInclude, visitedFilePaths, definedMacros, nullptr);
                outputStream << includedCode << "\n";
                continue;
            } else {
                throw std::runtime_error("Malformed #include directive in " + pathKeyStr + ": " + std::string(trimmedLine));
            }
        }

        if (currentActive) {
            outputStream << line << "\n";
        }
    }

    if (!conditionStack.empty()) {
        throw std::runtime_error("Unterminated conditional block in " + pathKeyStr);
    }
    // NOTE: pathKeyStr is NOT removed from visitedFilePaths here, as it tracks files processed
    // within the entire scope of one top-level preprocessWGSLFile/Inline
    return outputStream.str();
}

} // namespace


std::string preprocessWGSLFile(const std::filesystem::path& filePath,
                               const PlaceHoldersMap& placeholders,
                               const MacroDefinitions& macros)
{
    std::unordered_set<std::string> visitedFilePaths;
    // preprocessRecursive will read the file specified by filePath.
    const std::string combinedCode = preprocessRecursive(filePath, visitedFilePaths, macros, nullptr);
    const std::string finalCode = replacePlaceholders(combinedCode, placeholders);
    return finalCode;
}

std::string preprocessWGSLString(const std::string& shaderText,
                                 const PlaceHoldersMap& placeholders,
                                 const MacroDefinitions& macros)
{
    std::unordered_set<std::string> visitedFilePaths;
    // Use a conceptual path for the inline shader. Relative includes will be resolved
    // based on this path's parent.
    const std::filesystem::path inlineContextPath = std::filesystem::current_path() / "<inline_shader_context.wgsl>";
    const std::string combinedCode = preprocessRecursive(inlineContextPath, visitedFilePaths, macros, &shaderText);
    const std::string finalCode = replacePlaceholders(combinedCode, placeholders);
    return finalCode;
}

} // namespace MR::GPU
