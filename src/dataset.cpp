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