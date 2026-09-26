#include "types.hpp"
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <charconv>
#include <format>
#include <cmath>
#include <algorithm>
#include <ranges>

class Matrix
{
private:
    std::vector<double> data_;

    std::size_t rows_;
    std::size_t cols_;

    Layout layout_;

public:
    Matrix() = default;
    Matrix(std::size_t rows, std::size_t cols, Layout layout)
        : rows_(rows), cols_(cols), layout_(layout), data_(rows * cols, 0.0) {}

    double &operator()(
        std::size_t row,
        std::size_t column);

    const double &operator()(
        std::size_t row,
        std::size_t column) const;

    std::size_t rows() const { return rows_; }
    std::size_t cols() const { return cols_; }
    const std::vector<double> &data() const { return data_; }
    std::vector<double> &data() { return data_; }

    void save(const std::filesystem::path &path) const;
    static Matrix load(const std::filesystem::path &path);
};