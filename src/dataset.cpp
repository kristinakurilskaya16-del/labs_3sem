#include "dataset.hpp"
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

double quantile(const std::vector<double> &sorted_values, double p)
{
    if (sorted_values.empty())
    {
        return 0.0;
    }

    double pos = 1 + (sorted_values.size() - 1) * p;
    if (pos == std::floor(pos))
    {
        return sorted_values[static_cast<std::size_t>(pos)];
    }

    std::size_t lo = static_cast<std::size_t>(std::floor(pos));
    double f = pos - lo;

    return sorted_values[lo] + f * (sorted_values[lo + 1] - sorted_values[lo]);
}

// строка -> вектор
std::vector<std::string> split(
    const std::string &line,
    char delimiter)
{
    std::vector<std::string> tokens;
    std::string current;

    for (char c : line)
    {
        if (delimiter == c)
        {
            tokens.push_back(current);
            current.clear();
        }
        else
        {
            current += c;
        }
    }
    tokens.push_back(current);

    return tokens;
}

Dataset load_dataset(
    const std::filesystem::path &filepath,
    char delimiter, // разделитель
    const std::string &missing_marker)
{
    std::ifstream file(filepath); // почитать
    if (!file.is_open())
    {
        throw std::runtime_error("Не удалось открыть файл: " + filepath.string());
    }

    std::string line;

    if (!std::getline(file, line))
    {
        throw std::runtime_error("Файл пуст или не содержит заголовка");
    }

    if (!line.empty() && line.back() == '\r')
    {
        line.pop_back();
    }

    auto headers = split(line, delimiter);
    std::size_t num_cols = headers.size();

    if (num_cols == 0)
    {
        throw std::runtime_error("Заголовок не содержит столбцов");
    }

    std::vector<std::vector<std::string>> raw_columns(num_cols); // временное хранилище

    std::size_t current_row = 1;

    while (getline(file, line))
    {

        if (line.empty())
        {
            continue;
        }

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        auto tokens = split(line, delimiter);

        if (tokens.size() != num_cols)
        {
            throw std::runtime_error("Ошибка в строке" + std::to_string(current_row + 1) +
                                     "ожидалось " + std::to_string(num_cols) +
                                     "полей, найдено" + std::to_string(tokens.size()));
        }

        for (std::size_t j = 0; j < num_cols; ++j)
        {
            raw_columns[j].push_back(tokens[j]);
        }

        current_row++;
    }

    Dataset dataset;

    for (std::size_t j = 0; j < num_cols; ++j)
    {
        const std::string &col_name = headers[j];
        const auto &raw_data = raw_columns[j];

        bool is_numeric = true;

        for (const std::string &val : raw_data)
        {
            if (val == missing_marker || val.empty())
            {
                continue;
            }

            double test;
            auto [ptr, ec] = std::from_chars(val.data(), val.data() + val.size(), test);

            if (ec != std::errc{} || ptr != val.data() + val.size())
            {
                is_numeric = false;
                break;
            }
        }

        if (is_numeric)
        {
            NumericColumn num_col;

            for (const std::string &val : raw_data)
            {
                if (val.empty() || val == missing_marker)
                {
                    num_col.values.push_back(std::nullopt);
                }
                else
                {
                    double parsed;
                    std::from_chars(val.data(), val.data() + val.size(), parsed);
                    num_col.values.push_back(parsed);
                }
            }
            dataset.add_column(col_name, num_col);
        }
        else
        {
            CategoricalColumn cat_col;
            for (const std::string &val : raw_data)
            {
                if (val.empty() || val == missing_marker)
                {
                    cat_col.values.push_back(std::nullopt);
                }
                else
                {
                    cat_col.values.push_back(val);
                }
            }
            dataset.add_column(col_name, cat_col);
        }
    }
    if (!raw_columns.empty())
    {
        dataset.set_num_rows(raw_columns[0].size());
    }

    return dataset;
}

void Dataset::print_summary(std::ostream &out) const
{
    out << std::format("Объектов: {}\n", num_rows_);
    out << std::format("Признаков: {}\n\n", columns_.size());

    out << std::format("{:>4}{:<20}{:<30}{:^20}\n", "#", "Признак", "Тип", "Пропуски");

    std::size_t index = 0;
    std::size_t total_missing = 0;
    std::size_t cols_with_missing = 0;

    for (const auto &[name, col] : columns_)
    {
        std::string type_str;
        std::size_t missing_count = 0;

        if (std::holds_alternative<NumericColumn>(col))
        {
            type_str = "числовой";
            const auto &num_col = std::get<NumericColumn>(col); // получаем значение по типу

            for (const auto &val : num_col.values)
            {
                if (!val.has_value())
                {
                    missing_count++;
                }
            }
        }
        else
        {
            type_str = "категоиальный";
            const auto &cat_col = std::get<CategoricalColumn>(col);

            for (const auto &val : cat_col.values)
            {
                if (!val.has_value())
                {
                    missing_count++;
                }
            }
        }

        if (missing_count > 0)
        {
            double result = 100.0 * missing_count / num_rows_;
            out << std::format("{:>4}{:<20}{:<30} {} ({:.1f}%)\n",
                               index, name, type_str, missing_count, result);
            total_missing += missing_count;
            cols_with_missing++;
        }
        else
        {
            out << std::format("{:>4}{:<20}{:<30}{:^20}\n", index, name, type_str, 0);
        }

        index++;
    }

    out << std::format("\nВсего пропусков: {} в {} признаках\n", total_missing, cols_with_missing);
}

DatasetInfo Dataset::analyze() const
{
    DatasetInfo info;

    for (const auto &[name, col] : columns_)
    {
        if (std::holds_alternative<NumericColumn>(col))
        {
            const auto &num_col = std::get<NumericColumn>(col);
            NumericFeatureInfo num_info;

            std::vector<double> values;
            for (const auto &val : num_col.values)
            {
                if (val.has_value())
                {
                    values.push_back(*val);
                }
                else
                {
                    num_info.count_missing++;
                }
            }

            if (values.empty())
            {
                info[name] = num_info;
                continue;
            }

            auto [min_it, max_it] = std::ranges::minmax_element(values);
            num_info.minimum = *min_it;
            num_info.maximum = *max_it;

            double sum = 0.0;
            for (const auto &val : values)
            {
                sum += val;
            }
            num_info.medium = sum / values.size();

            double var_sum;
            for (const auto &val : values)
            {
                double diff = num_info.medium - val;
                var_sum += diff * diff;
            }
            num_info.dispersion = var_sum / values.size();

            std::ranges::sort(values);
            num_info.q05 = quantile(values, 0.05);
            num_info.q25 = quantile(values, 0.25);
            num_info.median = quantile(values, 0.5);
            num_info.q75 = quantile(values, 0.75);
            num_info.q95 = quantile(values, 0.95);

            info[name] = num_info;
        }
        else
        {
            const auto &cat_col = std::get<CategoricalColumn>(col);
            CategoricalFeatureInfo cat_info;

            for (const auto &val : cat_col.values)
            {
                if (val.has_value())
                {
                    const std::string &s = *val;
                    cat_info.frequencies[s]++;
                    if (cat_info.frequencies[s] == 1)
                    {
                        cat_info.categories.push_back(s);
                    }
                }
                else
                {
                    cat_info.count_missing++;
                }
            }

            info[name] = cat_info;
        }
    }
    return info;
}

void print_numeric_histogram(const std::vector<double>& values, int num_bins = 10) {
    
}
