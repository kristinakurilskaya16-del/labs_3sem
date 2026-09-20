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
