#pragma once

/** @file types.hpp
 * @brief Столбцы, статистики признаков и результат предобработки.
 */

#include "matrix.hpp"

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <unordered_map>

/** @brief Числовой столбец; std::nullopt обозначает пропуск. */
struct NumericColumn
{
    /** @brief Значения в порядке объектов датасета. */
    std::vector<std::optional<double>> values;
};

/** @brief Строковый категориальный столбец с явными пропусками. */
struct CategoricalColumn
{
    /** @brief Категории в порядке объектов; std::nullopt означает пропуск. */
    std::vector<std::optional<std::string>> values;
};

/** @brief Столбец одного из двух поддерживаемых типов. */
using Column = std::variant<NumericColumn, CategoricalColumn>;

/**
 * @brief Статистики числового признака без учёта пропусков.
 *
 * Все числовые поля по умолчанию инициализируются нулём.
 */
struct NumericFeatureInfo
{
    /** @brief Число пропусков. */
    std::size_t missing_count = 0;

    /** @brief Минимум наблюдаемых значений. */
    double min = 0.0;
    /** @brief Максимум наблюдаемых значений. */
    double max = 0.0;

    /** @brief Среднее без учёта пропусков. */
    double mean = 0.0;
    /** @brief Дисперсия с делением на число наблюдаемых значений. */
    double variance = 0.0;

    /** @brief Медиана (квантиль 0,5). */
    double median = 0.0;

    /** @brief Квантиль 0,05 с линейной интерполяцией. */
    double q05 = 0.0;
    /** @brief Квантиль 0,25. */
    double q25 = 0.0;
    /** @brief Квантиль 0,75. */
    double q75 = 0.0;
    /** @brief Квантиль 0,95. */
    double q95 = 0.0;
};

/** @brief Категории и частоты их появления без учёта пропусков. */
struct CategoricalFeatureInfo
{
    /** @brief Число пропусков. */
    std::size_t missing_count = 0;

    /** @brief Уникальные категории в порядке первого появления. */
    std::vector<std::string> categories;

    /** @brief Число появлений каждой категории. */
    std::unordered_map<std::string, std::size_t> frequencies; // для гистограммы
};

/** @brief Статистика числового или категориального признака. */
using FeatureInfo = std::variant<NumericFeatureInfo, CategoricalFeatureInfo>;

/** @brief Соответствие имени исходного признака и его статистики. */
using DatasetInfo = std::unordered_map<std::string, FeatureInfo>;

/**
 * @brief Результат преобразования датасета в числовую матрицу.
 *
 * При корректно сформированном результате количество элементов
 * feature_names совпадает с количеством столбцов matrix.
 */
struct TransformedDataset
{
    /** @brief Результат масштабирования и кодирования. */
    Matrix matrix;
    /** @brief Имя каждого столбца matrix в том же порядке. */
    std::vector<std::string> feature_names;
};
