#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <unordered_map>
#include <iostream>

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
    std::size_t missing_count;

    double min;
    double max;

    double mean;
    double variance;

    double median;

    double q05;
    double q25;
    double q75;
    double q95;
};

struct CategoricalFeatureInfo
{
    std::size_t missing_count;

    std::vector<std::string> categories;

    std::unordered_map<std::string, std::size_t> frequencies;
};

using FeatureInfo = std::variant<NumericFeatureInfo, CategoricalFeatureInfo>;

using DatasetInfo = std::unordered_map<std::string, FeatureInfo>;