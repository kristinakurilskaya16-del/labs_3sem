#pragma once

#include "matrix.hpp"

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <unordered_map>

struct NumericColumn
{
    std::vector<std::optional<double>> values;
};

struct CategoricalColumn
{
    std::vector<std::optional<std::string>> values;
};

using Column = std::variant<NumericColumn, CategoricalColumn>;

struct NumericFeatureInfo
{
    std::size_t missing_count = 0;

    double min = 0.0;
    double max = 0.0;

    double mean = 0.0;
    double variance = 0.0;

    double median = 0.0;

    double q05 = 0.0;
    double q25 = 0.0;
    double q75 = 0.0;
    double q95 = 0.0;
};

struct CategoricalFeatureInfo
{
    std::size_t missing_count = 0;

    std::vector<std::string> categories;

    std::unordered_map<std::string, std::size_t> frequencies; // для гистограммы
};

using FeatureInfo = std::variant<NumericFeatureInfo, CategoricalFeatureInfo>;

using DatasetInfo = std::unordered_map<std::string, FeatureInfo>;

struct TransformedDataset
{
    Matrix matrix;
    std::vector<std::string> feature_names;
};
