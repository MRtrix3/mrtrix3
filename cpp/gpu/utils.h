#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Utils {

// Divides the input vector into equal-sized rows (each row having "chunkSize" elements)
// and then performs a column-wise accumulation using the provided binary operator.
// e.g. { 1, 2, 3, 4, 5, 6 } with chunkSize = 2, we form the "matrix"
// [1, 3, 5]
// [2, 4, 6]
// and then perform the operation on each column.
template <typename T, typename BinaryOp>
std::vector<T> chunkReduce(const std::vector<T>& data, size_t chunkSize, BinaryOp op) {
    if (chunkSize == 0) {
        throw std::invalid_argument("chunkSize cannot be zero.");
    }
    if (data.size() % chunkSize != 0) {
        throw std::invalid_argument("vector size must be a multiple of chunkSize.");
    }

    const size_t numRows = data.size() / chunkSize;
    std::vector<T> result(chunkSize, T{});

    for (size_t row = 0; row < numRows; ++row) {
        for (size_t col = 0; col < chunkSize; ++col) {
            result[col] = op(result[col], data[row * chunkSize + col]);
        }
    }
    return result;
}

// Returns the smallest multiple of `multiple` that is greater or equal to `value`.
uint32_t nextMultipleOf(const uint32_t value, const uint32_t multiple);

enum ReadFileMode {
    Text,
    Binary
};
std::string readFile(const std::filesystem::path &filePath, ReadFileMode mode = ReadFileMode::Text);
}
