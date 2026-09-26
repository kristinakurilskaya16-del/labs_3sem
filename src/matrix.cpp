#include "matrix.hpp"
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

double &Matrix::operator()(std::size_t row, std::size_t col)
{
    if (layout_ == Layout::RowMajor)
    {
        return data_[row * cols_ + col];
    }
    else
    {
        return data_[col * rows_ + row];
    }
}

const double &Matrix::operator()(std::size_t row, std::size_t col) const
{
    if (layout_ == Layout::RowMajor)
    {
        return data_[row * cols_ + col];
    }
    else
    {
        return data_[col * rows_ + row];
    }
}

void Matrix::save(const std::filesystem::path &path) const
{
    std::ofstream file(path, std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Не удалось открыть файл для записи: " + path.string());
    }

    file.write(reinterpret_cast<const char *>(&rows_), sizeof(rows_));
    file.write(reinterpret_cast<const char *>(&cols_), sizeof(cols_));
    file.write(reinterpret_cast<const char *>(&layout_), sizeof(layout_));
    file.write(reinterpret_cast<const char *>(data_.data()), data_.size() * sizeof(double));
}

Matrix Matrix::load(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Не удалось открыть файл для чтения: " + path.string());
    }

    std::size_t rows, cols;
    Layout layout;

    file.read(reinterpret_cast<char *>(&rows), sizeof(rows));
    file.read(reinterpret_cast<char *>(&cols), sizeof(cols));
    file.read(reinterpret_cast<char *>(&layout), sizeof(layout));

    Matrix matrix(rows, cols, layout);
    file.read(reinterpret_cast<char *>(matrix.data().data()), matrix.data().size() * sizeof(double));
    return matrix;
}