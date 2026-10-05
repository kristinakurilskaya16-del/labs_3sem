#include "dataset.hpp"
#include "cxxopts.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <variant>
#include <vector>

namespace
{

    NumericImputation parse_imputation(const std::string &s)
    {
        if (s == "mean")
            return NumericImputation::Mean;
        if (s == "median")
            return NumericImputation::Median;
        if (s == "zero")
            return NumericImputation::Zero;
        throw std::runtime_error("Неизвестная стратегия импутации: '" + s + "'. " +
                                 "Допустимые значения: mean, median, zero");
    }

    Scaling parse_scaling(const std::string &s)
    {
        if (s == "minmax")
            return Scaling::MinMax;
        if (s == "standard")
            return Scaling::Standard;
        if (s == "robust")
            return Scaling::Robust;
        throw std::runtime_error("Неизвестный способ масштабирования: '" + s + "'. " +
                                 "Допустимые значения: minmax, standard, robust");
    }

    Encoding parse_encoding(const std::string &s)
    {
        if (s == "onehot")
            return Encoding::OneHot;
        if (s == "ordinal")
            return Encoding::Ordinal;
        throw std::runtime_error("Неизвестный способ кодирования: '" + s + "'. " +
                                 "Допустимые значения: onehot, ordinal");
    }

    bool read_line(std::string &out)
    {
        if (!std::getline(std::cin, out))
        {
            return false;
        }
        if (!out.empty() && out.back() == '\r')
        {
            out.pop_back();
        }
        return true;
    }

}

int main(int argc, char *argv[])
{
    cxxopts::Options options("lab1_app", "Загрузка, анализ и предобработка табличных данных");

    options.add_options()("input", "Путь к входному CSV-файлу",
                          cxxopts::value<std::string>()->default_value("data/data.csv"))("output", "Путь для сохранения бинарной матрицы",
                                                                                         cxxopts::value<std::string>()->default_value("data/transformed.bin"))("bins", "Число интервалов гистограммы",
                                                                                                                                                               cxxopts::value<int>()->default_value("10"))("impute", "Стратегия заполнения числовых пропусков: mean, median, zero",
                                                                                                                                                                                                           cxxopts::value<std::string>()->default_value("median"))("scaling", "Способ масштабирования числовых признаков: minmax, standard, robust",
                                                                                                                                                                                                                                                                   cxxopts::value<std::string>()->default_value("robust"))("encoding", "Способ кодирования категориальных признаков: onehot, ordinal",
                                                                                                                                                                                                                                                                                                                           cxxopts::value<std::string>()->default_value("onehot"))("help", "Вывести справку");

    try
    {
        auto result = options.parse(argc, argv);

        if (result.count("help"))
        {
            std::cout << options.help() << "\n";
            return 0;
        }

        std::string input_path = result["input"].as<std::string>();
        std::string output_path = result["output"].as<std::string>();
        int num_bins = result["bins"].as<int>();

        if (num_bins <= 0)
        {
            std::cerr << "Ошибка: --bins должно быть положительным, получено " << num_bins << "\n";
            return 1;
        }

        NumericImputation impute_strategy = parse_imputation(result["impute"].as<std::string>());
        Scaling scaling_strategy = parse_scaling(result["scaling"].as<std::string>());
        Encoding encoding_strategy = parse_encoding(result["encoding"].as<std::string>());

        Dataset dataset = load_dataset(input_path);
        dataset.print_summary();

        DatasetInfo info = dataset.analyze();
        std::vector<std::string> feature_names = dataset.get_feature_names();

        while (true)
        {
            std::cout << "\nВведите номер признака (или 'stop' для выхода): ";

            std::string input;
            if (!read_line(input))
            {
                std::cout << "\n";
                break;
            }

            if (input == "stop")
                break;

            int index = 0;
            auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), index);

            if (ec != std::errc{} || ptr != input.data() + input.size())
            {
                std::cout << "Некорректный ввод. Введите число или 'stop'.\n";
                continue;
            }

            if (index < 0 || index >= static_cast<int>(feature_names.size()))
            {
                std::cout << "Номер вне диапазона. Доступные значения: 0-"
                          << static_cast<int>(feature_names.size()) - 1 << "\n";
                continue;
            }

            const std::string &name = feature_names[index];
            const Column &col = dataset.get_columns().at(name);
            const FeatureInfo &fi = info[name];

            if (std::holds_alternative<NumericFeatureInfo>(fi))
            {
                const auto &num_info = std::get<NumericFeatureInfo>(fi);
                const auto &num_col = std::get<NumericColumn>(col);

                std::vector<double> clean_values;
                clean_values.reserve(num_col.values.size());

                for (const auto &v : num_col.values)
                {
                    if (v.has_value())
                        clean_values.push_back(*v);
                }

                print_numeric_info(index, name, num_info, clean_values, num_bins);
            }
            else
            {
                const auto &cat_info = std::get<CategoricalFeatureInfo>(fi);
                print_categorical_info(index, name, cat_info);
            }
        }

        std::cout << "\nХотите сохранить преобразованный датасет? (yes/no): ";
        std::string answer;

        if (!read_line(answer))
        {
            return 0;
        }

        if (answer == "yes" || answer == "y")
        {
            dataset.impute(info, impute_strategy);

            TransformedDataset transformed = dataset.transform(info, scaling_strategy, encoding_strategy);

            transformed.matrix.save(output_path);

            std::filesystem::path names_path = std::filesystem::path(output_path).parent_path() / "feature_names.txt";
            std::ofstream names_file(names_path);

            if (!names_file)
            {
                std::cerr << "Предупреждение: не удалось открыть " << names_path
                          << " для записи имён признаков\n";
            }
            else
            {
                for (const auto &fname : transformed.feature_names)
                {
                    names_file << fname << "\n";
                }
            }

            std::cout << "Сохранено! Размер: " << transformed.matrix.rows() << " x "
                      << transformed.matrix.cols() << "\n";

            std::cout
                << "CSV size:    "
                << std::filesystem::file_size(input_path)
                << " bytes\n";

            std::cout
                << "Binary size: "
                << std::filesystem::file_size(output_path)
                << " bytes\n";
        }

        return 0;
    }
    catch (const cxxopts::exceptions::exception &e)
    {
        std::cerr << "Ошибка разбора аргументов командной строки: " << e.what() << "\n";
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
}