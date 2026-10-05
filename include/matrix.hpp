#pragma once

#include "enums.hpp"

#include <string>
#include <vector>
#include <filesystem>

class Matrix
{
private:
    std::vector<double> data_;

    std::size_t rows_ = 0;
    std::size_t cols_ = 0;

    Layout layout_ = Layout::RowMajor;

public:
    Matrix() = default;

    Matrix(std::size_t rows, std::size_t cols, Layout layout)
        : data_(rows * cols, 0.0), rows_(rows), cols_(cols), layout_(layout) {}

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
    static Matrix load(const std::filesystem::path &path); // Named Constructor Idiom / несколько "конструкторов"
};