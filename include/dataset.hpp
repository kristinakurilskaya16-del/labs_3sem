#pragma once
/** @file dataset.hpp
 * @brief Датасет, загрузчик CSV и функции анализа, преобразования и вывода.
 */
#include "types.hpp"
#include <span>
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <iostream>
#include <utility>

/**
 * @brief Столбцовое хранилище данных с сохранением порядка признаков.
 * Пропуски представлены std::nullopt. Статистики хранятся отдельно в DatasetInfo.
 * Объект без добавленных столбцов имеет ноль строк.
 */
class Dataset
{
private:
    std::unordered_map<std::string, Column> columns_;
    std::vector<std::string> feature_order_; // порядок, как в исходном датасете

public:
    /**
     * @brief Копирует столбец в датасет и запоминает порядок добавления.
     * @param name Непустое уникальное имя.
     * @param col Столбец той же длины, что уже добавленные столбцы.
     * @throws std::runtime_error Если имя пустое, повторяется или длина не совпадает.
     */
    void add_column(const std::string &name, const Column &col);

    /** @brief Предоставляет столбцы для чтения.
     * @return Ссылка на таблицу имён и столбцов; её обход не сохраняет порядок CSV.
     */
    const std::unordered_map<std::string, Column> &get_columns() const
    {
        return columns_;
    }

    /** @brief Возвращает число объектов.
     * @return Длина столбца или ноль, если столбцов нет.
     */
    std::size_t get_num_rows() const;

    /**
     * @brief Выводит размеры, типы признаков и количества пропусков.
     * @param out Поток вывода; по умолчанию std::cout.
     * @note При нуле строк выводит сообщение о пустом датасете вместо таблицы.
     */
    void print_summary(std::ostream &out = std::cout) const;

    /**
     * @brief Вычисляет статистики признаков без изменения датасета.
     *
     * @return Статистики числовых и категориальных признаков.
     *
     * @pre Непропущенные числовые значения должны быть конечными.
     *
     * @note Конечность чисел отдельно не проверяется.
     * Пропуски не участвуют в вычислениях.
     * Дисперсия делится на число наблюдаемых значений.
     * Для полностью пропущенного числового столбца
     * числовые статистики остаются нулевыми.
     */
    DatasetInfo analyze() const;

    /**
     * @brief Заменяет пропуски, изменяя столбцы текущего датасета.
     * @param info Ранее вычисленные статистики с совпадающими именами и типами столбцов.
     * @param strategy Способ заполнения числовых пропусков.
     * @throws std::bad_variant_access Если для числового столбца
     * передана статистика другого типа.
     * @throws std::out_of_range Если в info нет имени столбца.
     * @note Статистики не пересчитываются. Категориальные пропуски становятся
     * строкой __MISSING__; переданные статистики остаются неизменными.
     */
    void impute(const DatasetInfo &info, NumericImputation strategy);

    /**
     * @brief Преобразует все столбцы в плотную матрицу без изменения датасета.
     * @param info Статистики с совпадающими именами и типами, обычно обучающей выборки.
     * @param scaling Способ масштабирования числовых значений.
     * @param encoding Способ кодирования категорий.
     * @param layout Физический порядок результата.
     * @return Матрица и имена столбцов в исходном порядке с расширением OneHot.
     * @throws std::invalid_argument Если датасет не содержит столбцов.
     * @throws std::bad_variant_access Если тип статистики
     * не соответствует типу столбца.
     * @throws std::out_of_range Если в info нет нужного имени.
     * @note Использует ту же обработку пропусков и неизвестных категорий,
     * что и перегрузка с included_columns; рекомендуется предварительно вызвать impute().
     */
    TransformedDataset transform(
        const DatasetInfo &info,
        Scaling scaling,
        Encoding encoding,
        Layout layout = Layout::RowMajor) const;

    /**
     * @brief Преобразует выбранные столбцы с готовыми статистиками.
     * @param info Статистики для выбранных имён; категории задаются этим словарём.
     * @param scaling Способ масштабирования; None оставляет числа без изменения.
     * @param encoding OneHot или Ordinal; категории сортируются лексикографически.
     * @param layout Физический порядок результата.
     * @param included_columns Непустое множество существующих имён. Порядок берётся из датасета.
     * @return Матрица и соответствующие имена признаков.
     * @throws std::bad_variant_access Если тип статистики
     * не соответствует типу столбца.
     * @throws std::invalid_argument Если выбор пуст или содержит неизвестное имя.
     * @throws std::out_of_range Если в info нет выбранного имени.
     * @note Оставшиеся числовые пропуски дают ноль. Категориальный пропуск или
     * неизвестная категория дают все нули в OneHot либо код 0 в Ordinal.
     * __MISSING__ включается в список категорий, если missing_count в info > 0;
     * совпадение этой строки с существующей категорией отдельно не устраняется.
     */
    TransformedDataset transform(
        const DatasetInfo &info,
        Scaling scaling,
        Encoding encoding,
        Layout layout,
        const std::unordered_set<std::string> &included_columns) const;

    /**
     * @brief Делит объекты на обучающую и тестовую части, не меняя исходные данные.
     * @param train_ratio Доля обучающей части строго между 0 и 1.
     * @return Пара: обучающая и тестовая части с теми же именами и порядком столбцов.
     * @throws std::invalid_argument Если доля вне (0, 1), в том числе NaN, или одна часть пуста.
     * @throws std::runtime_error Если нет столбцов или их длины различаются.
     * @note Число обучающих строк округляется вниз. Используется перемешивание
     * с фиксированным seed 50; разделение не стратифицировано по классам.
     */
    std::pair<Dataset, Dataset> split(double train_ratio) const;

    /** @brief Возвращает имена исходных признаков в порядке добавления.
     * @return Копия списка имён, не имён после OneHot.
     */
    std::vector<std::string> get_feature_names() const
    {
        return feature_order_;
    }
};

/**
 * @brief Разделяет строку по каждому символу-разделителю.
 * @param line Исходная строка без перевода строки.
 * @param delimiter Разделитель полей.
 * @return Поля, включая пустые и последнее пустое поле; для пустой строки — одно пустое поле.
 * @note Кавычки не защищают разделители внутри поля; это не универсальный CSV-разбор.
 */
std::vector<std::string> split(
    const std::string &line,
    char delimiter = ',');

/**
 * @brief Загружает CSV выбранного формата с заголовком и явными пропусками.
 * @param filepath Входной файл.
 * @param delimiter Разделитель, по умолчанию запятая.
 * @param missing_marker Содержимое поля, обозначающее пропуск, по умолчанию NA.
 * @return Датасет с автоматически определёнными типами и исходным порядком столбцов.
 * @throws std::runtime_error При ошибке открытия, отсутствии заголовка,
 * несогласованном числе полей, повторяющихся именах или ошибке числового разбора.
 * @note Внешняя пара кавычек снимается; пустое поле тоже считается пропуском.
 * Пустое имя получает имя id. Запятые, переводы строк внутри полей и экранированные
 * кавычки не поддерживаются. Полностью пропущенный столбец считается числовым.
 * Пустые LF-строки пропускаются и не учитываются в счётчике строк диагностики.
 */
Dataset load_dataset(
    const std::filesystem::path &filepath,
    char delimiter = ',',
    const std::string &missing_marker = "NA");

/**
 * @brief Вычисляет квантиль с линейной интерполяцией по позиции (n - 1) * p.
 * @param sorted_values Невладеющее представление отсортированных по возрастанию чисел.
 * @param p Уровень квантиля от 0 до 1 включительно.
 * @return Квантиль либо 0 для пустого массива.
 * @pre Данные конечны, отсортированы, p не NaN; эти условия отдельно не проверяются.
 * @throws std::invalid_argument Если p < 0 или p > 1.
 */
double quantile(
    std::span<const double> sorted_values,
    double p);

/**
 * @brief Печатает частотную гистограмму в std::cout.
 * @param values Наблюдаемые конечные значения без пропусков.
 * @param min Нижняя граница значений.
 * @param max Верхняя граница значений.
 * @param num_bins Число интервалов.
 * @pre Если выборка непустая и содержит разные значения,
 * min < max и все значения находятся в [min, max].
 * @note При num_bins <= 0, пустых values или min == max печатает сообщение
 * вместо гистограммы. Максимум включён в последний интервал, хотя подпись содержит скобку ).
 */
void print_numeric_histogram(
    std::span<const double> values,
    double min,
    double max,
    int num_bins = 10);

/**
 * @brief Печатает частоты категорий в std::cout.
 * @param cat_info Категории и их частоты.
 * @pre Для каждой категории есть положительная частота, представимая типом int.
 * @throws std::out_of_range Если категория отсутствует в frequencies.
 * @note При пустом frequencies печатает сообщение вместо гистограммы.
 */
void print_categorical_histogram(const CategoricalFeatureInfo &cat_info);

/**
 * @brief Печатает статистику числового признака и его гистограмму в std::cout.
 * @param index Номер признака для подписи.
 * @param name Имя признака.
 * @param info Готовая статистика признака.
 * @param clean_values Наблюдаемые конечные значения без пропусков, соответствующие info.
 * @param num_bins Число интервалов гистограммы.
 * @note Для пустых clean_values сообщает о недоступной статистике.
 */
void print_numeric_info(
    std::size_t index,
    const std::string &name,
    const NumericFeatureInfo &info,
    std::span<const double> clean_values,
    int num_bins = 10);

/**
 * @brief Печатает пропуски, категории и гистограмму в std::cout.
 * @param index Номер признака для подписи.
 * @param name Имя признака.
 * @param info Согласованные категории и частоты исходного признака.
 * @throws std::out_of_range Если категория отсутствует в frequencies при выводе гистограммы.
 * @note При отсутствии категорий печатает сообщение вместо гистограммы.
 */
void print_categorical_info(
    std::size_t index,
    const std::string &name,
    const CategoricalFeatureInfo &info);

/**
 * @brief Масштабирует одно число по готовым статистикам.
 * @param x Исходное конечное значение.
 * @param info Статистики исходного признака; для Standard дисперсия неотрицательна.
 * @param scaling Стратегия масштабирования.
 * @return Преобразованное число; для None исходное x, при нулевом знаменателе — 0.
 * @note Проверка конечности чисел и корректности статистик не выполняется.
 */
double scale_value(
    double x,
    const NumericFeatureInfo &info,
    Scaling scaling);

/**
 * @brief Добавляет кодированные столбцы и имена в переданные контейнеры.
 * @param values Категории по объектам с возможными пропусками.
 * @param sorted_categories Упорядоченный список категорий; функция его не сортирует.
 * @param encoding OneHot или Ordinal.
 * @param name Исходное имя признака для результирующих имён.
 * @param result_columns Контейнер, в конец которого добавляются числовые столбцы.
 * @param feature_names Контейнер для имён добавленных столбцов.
 * @note Неизвестное или отсутствующее значение даёт нули в OneHot либо код 0
 * в Ordinal. OneHot использует имя вида name_category, Ordinal — исходное name.
 * Исходное содержимое выходных контейнеров сохраняется.
 */
void encode_categorial(
    const std::vector<std::optional<std::string>> &values,
    const std::vector<std::string> &sorted_categories,
    Encoding encoding,
    const std::string &name,
    std::vector<std::vector<double>> &result_columns,
    std::vector<std::string> &feature_names);

/**
 * @brief Снимает одну внешнюю пару двойных кавычек.
 * @param s Исходная строка.
 * @return Строка без крайних кавычек, если обе есть, иначе копия s.
 * @note Не проверяет баланс кавычек и не обрабатывает экранирование.
 */
inline std::string strip_quotes(const std::string &s)
{
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
    {
        return s.substr(1, s.size() - 2);
    }
    return s;
}
