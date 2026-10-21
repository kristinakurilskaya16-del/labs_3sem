#include "dataset.hpp"
#include "decision_stump.hpp"

#include <iostream>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <variant>
#include <vector>
#include <utility>

int main()
{
    Dataset dataset = load_dataset("data/data.csv");

    auto [train_dataset, test_dataset] = dataset.split(0.8);

    std::unordered_set<std::string> included_columns;

    for (const auto &name : dataset.get_feature_names())
    {
        if (name != "id" && name != "Category")
        {
            included_columns.insert(name);
        }
    }

    std::unordered_map<std::string, double> label_to_number;

    std::vector<double> y_train;
    std::vector<double> y_test;

    const auto &train_categories =
        std::get<CategoricalColumn>(
            train_dataset.get_columns().at("Category"))
            .values;

    const auto &test_categories =
        std::get<CategoricalColumn>(
            test_dataset.get_columns().at("Category"))
            .values;

    for (const auto &category : train_categories)
    {
        if (!category.has_value())
        {
            throw std::runtime_error(
                "Пропущенное значение в Category");
        }

        const std::string &name = *category;

        if (!label_to_number.contains(name))
        {
            double label = static_cast<double>(label_to_number.size());
            label_to_number.emplace(name, label);
        }

        y_train.push_back(label_to_number.at(name));
    }

    for (const auto &category : test_categories)
    {
        if (!category.has_value())
        {
            throw std::runtime_error(
                "Пропущенное значение в Category");
        }

        const std::string &name = *category;

        if (!label_to_number.contains(name))
        {
            throw std::runtime_error(
                "В тестовой выборке обнаружен неизвестный класс");
        }

        y_test.push_back(label_to_number.at(name));
    }

    auto train_info = train_dataset.analyze();
    auto test_info = test_dataset.analyze();

    train_dataset.impute(
        train_info,
        NumericImputation::Median);

    test_dataset.impute(
        test_info,
        NumericImputation::Median);

    auto transformed_train = train_dataset.transform(
        train_info,
        Scaling::Robust,
        Encoding::OneHot,
        Layout::RowMajor,
        included_columns);

    auto transformed_test = test_dataset.transform(
        train_info,
        Scaling::Robust,
        Encoding::OneHot,
        Layout::RowMajor,
        included_columns);

    if (transformed_train.feature_names != transformed_test.feature_names)
    {
        throw std::runtime_error(
            "Признаки обучающей и тестовой выборок не совпадают");
    }

    Matrix X_train = std::move(transformed_train.matrix);
    Matrix X_test = std::move(transformed_test.matrix);

    DecisionStump model(SplitCriterion::Gini, 1);

    model.fit(X_train, y_train);

    std::vector<double> predictions = model.predict(X_test);

    std::size_t correct = 0;

    for (std::size_t i = 0; i < y_test.size(); ++i)
    {
        if (predictions[i] == y_test[i])
        {
            ++correct;
        }
    }

    double accuracy =
        static_cast<double>(correct) / y_test.size();

    std::cout << "Accuracy: " << accuracy << '\n';

    return 0;
}