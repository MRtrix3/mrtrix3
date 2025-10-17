#include "utils.h"

#include <cstdint>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace std::string_literals;

uint32_t Utils::nextMultipleOf(const uint32_t value, const uint32_t multiple)
{
    if (value > std::numeric_limits<uint32_t>::max() - multiple) {
        return std::numeric_limits<uint32_t>::max();
    }
    return (value + multiple - 1) / multiple * multiple;
}

std::string Utils::readFile(const std::filesystem::path &filePath, ReadFileMode mode)
{
    if(!std::filesystem::exists(filePath)) {
        throw std::runtime_error("File not found: "s + filePath.string());
    }

    const auto openMode = (mode == ReadFileMode::Binary) ? std::ios::in | std::ios::binary : std::ios::in;
    std::ifstream f(filePath, std::ios::in | openMode);
    const auto fileSize = std::filesystem::file_size(filePath);
    std::string result(fileSize, '\0');
    f.read(result.data(), fileSize);

    return result;
}

