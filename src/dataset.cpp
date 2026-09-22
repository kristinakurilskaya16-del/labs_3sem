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

std::vector<std::string> Dataset::get_feature_names() const
{
    std::vector<std::string> names;

    for (const auto &[name, col] : columns_)
    {
        names.push_back(name);
    }
    return names;
}

std::size_t Dataset::get_num_rows() const
{
    if (columns_.empty())
    {
        return 0;
    }

    const auto &first_col = columns_.begin()->second; // begin - ключ, second - значение

    if (std::holds_alternative<NumericColumn>(first_col))
    {
        return std::get<NumericColumn>(first_col).values.size();
    }
    else
    {
        return std::get<CategoricalColumn>(first_col).values.size();
    }
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
    char delimiter,
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

    return dataset;
}

void Dataset::print_summary(std::ostream &out) const
{
    out << std::format("Объектов: {}\n", get_num_rows());
    out << std::format("Признаков: {}\n\n", columns_.size());

    out << std::format("{:>4}{:<20}{:<30}{:^20}\n", "#", "Признак", "Тип", "Пропуски");

    std::size_t index = 0;
    std::size_t total_missing = 0;
    std::size_t cols_with_missing = 0;
    std::size_t num_rows = get_num_rows();

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
            double result = 100.0 * missing_count / num_rows;
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
                    values.push_back(*val); // разыменование std::optional
                }
                else
                {
                    num_info.missing_count++;
                }
            }

            if (values.empty())
            {
                info[name] = num_info;
                continue;
            }

            auto [min_it, max_it] = std::ranges::minmax_element(values);
            num_info.min = *min_it;
            num_info.max = *max_it;

            double sum = 0.0;
            for (const auto &val : values)
            {
                sum += val;
            }
            num_info.mean = sum / values.size();

            double var_sum;
            for (const auto &val : values)
            {
                double diff = num_info.mean - val;
                var_sum += diff * diff;
            }
            num_info.variance = var_sum / values.size();

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
                    cat_info.missing_count++;
                }
            }

            info[name] = cat_info;
        }
    }
    return info;
}

void print_numeric_histogram(std::span<const double> values, double min, double max, int num_bins = 10)
{
    if (values.empty() || min == max)
    {
        std::cout << "Невозможно построить гистограмму.";
        return;
    }

    double bin_width = (max - min) / num_bins;

    std::vector<int> counts(num_bins, 0); // (размер, начальное значение)

    for (double v : values)
    {
        int bin = static_cast<int>((v - min) / bin_width); // смещаем минимум -> 0

        if (bin >= num_bins)
        {
            bin = num_bins - 1;
        }

        counts[bin]++;
    }

    int max_count = *std::ranges::max_element(counts); // для масштаба

    for (int i = 0; i < num_bins; ++i)
    {
        double bin_start = min + i * bin_width;
        double bin_end = bin_start + bin_width;

        int bar_length = static_cast<int>(30.0 * counts[i] / max_count);

        std::string bar(bar_length, '#');
        std::cout << std::format("  [{:<8.2f}, {:<8.2f})  {:<30}  {}\n",
                                 bin_start, bin_end, bar, counts[i]);
    }
}

void print_categorical_histogram(const CategoricalFeatureInfo &cat_info)
{
    if (cat_info.frequencies.empty())
    {
        std::cout << "Невозможно построить гистограмму.";
        return;
    }

    int max_freq = 0;
    for (const auto &[cat, freq] : cat_info.frequencies)
    {
        if (freq > max_freq)
        {
            max_freq = freq;
        }
    }

    for (const auto &cat : cat_info.categories)
    {
        int freq = cat_info.frequencies.at(cat); // поиск значения по ключу

        int bar_length = static_cast<int>(30.0 * freq / max_freq);

        std::string bar(bar_length, '#');
        std::cout << std::format("  {:<25}  {:<30}  {}\n", cat, bar, freq);
    }
}

void print_numeric_info(std::size_t index, const std::string &name, const NumericFeatureInfo &info, std::span<const double> clean_values)
{
    std::cout << std::format("\nПризнак {}: {} (числовой)\n", index, name);
    std::cout << std::format("  пропусков: {}\n", info.missing_count);

    if (clean_values.empty())
    {
        std::cout << "  (Все значения пропущены, статистика недоступна)\n";
        return;
    }

    std::cout << std::format("  min: {:.3f}    max: {:.3f}\n", info.min, info.max);
    std::cout << std::format("  mean: {:.3f}   var: {:.3f}\n", info.mean, info.variance);
    std::cout << std::format("  q05: {:.3f}\n", info.q05);
    std::cout << std::format("  q25: {:.3f}\n", info.q25);
    std::cout << std::format("  q50: {:.3f}\n", info.median);
    std::cout << std::format("  q75: {:.3f}\n", info.q75);
    std::cout << std::format("  q95: {:.3f}\n", info.q95);

    std::cout << "\n  Гистограмма:\n";
    print_numeric_histogram(clean_values, info.min, info.max, 10);
}

void print_categorical_info(std::size_t index, const std::string &name, const CategoricalFeatureInfo &info)
{
    std::cout << std::format("\nПризнак {}: {} (категориальный)\n", index, name);
    std::cout << std::format("  пропусков: {}\n", info.missing_count);

    if (info.categories.empty())
    {
        std::cout << "  (Нет категорий)\n";
        return;
    }

    print_categorical_histogram(info);
}