#include "dataset.hpp"
#include "matrix.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <cassert>
#include <string>

namespace fs = std::filesystem;

void test_split_with_missing()
{
    std::string line = "1,NA,3.14,hello";
    auto tokens = split(line, ',');

    assert(tokens.size() == 4);
    assert(tokens[0] == "1");
    assert(tokens[1] == "NA");
    assert(tokens[2] == "3.14");
    assert(tokens[3] == "hello");
}

void test_type_detection()
{
    fs::path temp_csv = "test_type_detection.csv";
    {
        std::ofstream file(temp_csv);
        file << "numeric,categorical\n";
        file << "1.0,red\n";
        file << "2.0,blue\n";
        file << "3.0,green\n";
        file << "NA,yellow\n";
    }

    auto dataset = load_dataset(temp_csv, ',', "NA");
    auto columns = dataset.get_columns();

    assert(columns.size() == 3);
    assert(std::holds_alternative<NumericColumn>(columns.at("numeric")));
    assert(std::holds_alternative<CategoricalColumn>(columns.at("categorical")));

    fs::remove(temp_csv);
}

void test_quantile()
{
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0};

    assert(std::abs(quantile(values, 0.5) - 3.0) < 1e-9);
    assert(std::abs(quantile(values, 0.25) - 2.0) < 1e-9);
    assert(std::abs(quantile(values, 0.75) - 4.0) < 1e-9);
    assert(std::abs(quantile(values, 0.05) - 1.2) < 1e-9);
    assert(std::abs(quantile(values, 0.95) - 4.8) < 1e-9);
}

void test_categorical_encoding()
{
    std::vector<std::optional<std::string>> values = {"red", "blue", "red", "green"};
    std::vector<std::string> sorted_categories = {"blue", "green", "red"};

    // Ordinal
    {
        std::vector<std::vector<double>> result_columns;
        std::vector<std::string> feature_names;

        encode_categorial(values, sorted_categories, Encoding::Ordinal, "color",
                          result_columns, feature_names);

        assert(result_columns.size() == 1);
        assert(feature_names[0] == "color");
        assert(std::abs(result_columns[0][0] - 2.0) < 1e-9); // red -> 2
        assert(std::abs(result_columns[0][1] - 0.0) < 1e-9); // blue -> 0
        assert(std::abs(result_columns[0][2] - 2.0) < 1e-9); // red -> 2
        assert(std::abs(result_columns[0][3] - 1.0) < 1e-9); // green -> 1
    }

    // One-Hot
    {
        std::vector<std::vector<double>> result_columns;
        std::vector<std::string> feature_names;

        encode_categorial(values, sorted_categories, Encoding::OneHot, "color",
                          result_columns, feature_names);

        assert(result_columns.size() == 3);
        assert(feature_names[0] == "color_blue");
        assert(feature_names[1] == "color_green");
        assert(feature_names[2] == "color_red");

        // Первая строка "red": [0, 0, 1]
        assert(std::abs(result_columns[0][0] - 0.0) < 1e-9);
        assert(std::abs(result_columns[1][0] - 0.0) < 1e-9);
        assert(std::abs(result_columns[2][0] - 1.0) < 1e-9);

        // Вторая строка "blue": [1, 0, 0]
        assert(std::abs(result_columns[0][1] - 1.0) < 1e-9);
        assert(std::abs(result_columns[1][1] - 0.0) < 1e-9);
        assert(std::abs(result_columns[2][1] - 0.0) < 1e-9);
    }
}

void test_matrix_save_load()
{
    fs::path temp_bin = "test_matrix.bin";

    Matrix original(3, 4, Layout::RowMajor);

    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 4; ++j)
            original(i, j) = i * 10.0 + j;

    original.save(temp_bin);
    assert(fs::exists(temp_bin)); // файл существует

    Matrix loaded = Matrix::load(temp_bin);
    assert(loaded.rows() == 3);
    assert(loaded.cols() == 4);

    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 4; ++j)
            assert(std::abs(loaded(i, j) - (i * 10.0 + j)) < 1e-9);

    fs::remove(temp_bin);
}

void test_inconsistent_csv()
{
    fs::path temp_csv = "test_inconsistent.csv";
    {
        std::ofstream file(temp_csv);
        file << "a,b,c\n";
        file << "1,2,3\n";
        file << "4,5\n";
    }

    bool exception_thrown = false;
    try
    {
        auto dataset = load_dataset(temp_csv, ',', "NA");
    }
    catch (const std::runtime_error &e)
    {
        exception_thrown = true;
        std::string msg = e.what();
        // Проверяем, что в сообщении есть номер строки
        assert(msg.find("строке") != std::string::npos ||
               msg.find("строка") != std::string::npos);
    }

    assert(exception_thrown);
    (void)exception_thrown;

    fs::remove(temp_csv);
}

int main()
{
    test_split_with_missing();
    test_type_detection();
    test_quantile();
    test_categorical_encoding();
    test_matrix_save_load();
    test_inconsistent_csv();

    std::cout << "Все тесты пройдены\n";
    return 0;
}