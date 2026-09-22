#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <unordered_map>
#include "dataset.hpp"

int main()
{
    Dataset dataset = load_dataset("data/data.csv");
    dataset.print_summary();

    DatasetInfo info = dataset.analyze();
    std::vector<std::string> feature_names = dataset.get_feature_names();

    int number;
    while (true)
    {
        std::cout << "\nВведите номер признака(или 'stop' для выхода): ";

        std::string input;
        std::getline(std::cin, input);

        if (input.empty() && input.back() == '\r')
        {
            input.pop_back();
        }

        if (input == "stop")
        {
            std::cout << "Необходимо ли сохранить преобразованный датасет?('yes' - да, 'no' - нет)";
            break;
        }

        int index;
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

        const std::string &name = feature_names[index];     // название
        const Column &col = dataset.get_columns().at(name); // столбец
        const FeatureInfo &fi = info[name];                 // статистика

        if (std::holds_alternative<NumericFeatureInfo>(fi))
        {
            const auto &num_info = std::get<NumericFeatureInfo>(fi); // сами данные
            const auto &num_col = std::get<NumericColumn>(col);

            std::vector<double> clean_values;
            clean_values.reserve(num_col.values.size());

            for (const auto &v : num_col.values)
            {
                if (v.has_value())
                    clean_values.push_back(*v);
            }

            print_numeric_info(index, name, num_info, clean_values);
        }
        else
        {
            const auto &cat_info = std::get<CategoricalFeatureInfo>(fi);
            print_categorical_info(index, name, cat_info);
        }
    }

    return 0;
}