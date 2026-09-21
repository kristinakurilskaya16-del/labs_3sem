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
    std::size_t count_missing = 0;
    double minimum = 0.0;
    double maximum = 0.0;
    double medium = 0.0;
    double dispersion = 0.0;
    double median = 0.0;
    double q05 = 0.0;
    double q25 = 0.0;
    double q75 = 0.0;
    double q95 = 0.0;
};

struct CategoricalFeatureInfo
{
    std::size_t count_missing = 0;
    std::vector<std::string> categories;
    std::unordered_map<std::string, std::size_t> frequencies;
};

using FeatureInfo = std::variant<NumericFeatureInfo, CategoricalFeatureInfo>;
using DatasetInfo = std::unordered_map<std::string, FeatureInfo>;