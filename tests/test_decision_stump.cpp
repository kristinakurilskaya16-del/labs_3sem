#include "decision_stump.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <random>

namespace
{
    bool close(double a, double b)
    {
        return std::abs(a - b) <= 1e-10 * std::max({1.0, std::abs(a), std::abs(b)});
    }

    struct Reference
    {
        bool split = false;

        std::size_t feature = 0;
        double threshold = 0;

        double left = 0;
        double right = 0;

        double score = -std::numeric_limits<double>::infinity();
    };

    double leaf_value(const std::vector<double> &values, SplitCriterion criterion)
    {
        assert(!values.empty());

        if (criterion == SplitCriterion::Variance)
            return std::accumulate(values.begin(), values.end(), 0.0) / values.size();

        std::map<double, std::size_t> counts;

        for (double value : values)
            ++counts[value];

        double best = counts.begin()->first;

        std::size_t frequency = 0;
        for (const auto &[value, count] : counts)
            if (count > frequency)
            {
                frequency = count;
                best = value;
            }

        return best;
    }

    double contribution(const std::vector<double> &values, SplitCriterion criterion)
    {
        assert(!values.empty());

        if (criterion == SplitCriterion::Variance)
        {
            const double sum = std::accumulate(values.begin(), values.end(), 0.0);
            return sum * sum / values.size();
        }

        std::map<double, std::size_t> counts;
        for (double value : values)
            ++counts[value];

        double squared = 0;
        for (const auto &[value, count] : counts)
        {
            (void)value; // переменная не используется
            squared += static_cast<double>(count) * count;
        }

        return squared / values.size();
    }

    Reference naive_fit(const Matrix &x,
                        std::span<const double> y,
                        SplitCriterion criterion,
                        std::size_t min_leaf)
    {
        if (!min_leaf || !x.rows() || !x.cols() || y.size() != x.rows())
            throw std::invalid_argument("Некорректные входные данные эталона");

        Reference result;

        const std::vector<double> all(y.begin(), y.end());

        result.left = result.right = leaf_value(all, criterion);

        if (criterion == SplitCriterion::Gini &&
            std::all_of(y.begin(), y.end(), [&](double v)
                        { return v == y.front(); }))
            return result;

        for (std::size_t j = 0; j < x.cols(); ++j)
        {
            std::vector<double> unique;
            for (std::size_t i = 0; i < x.rows(); ++i)
                unique.push_back(x(i, j));

            std::sort(unique.begin(), unique.end());
            unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
            // перебираем пары
            for (std::size_t k = 1; k < unique.size(); ++k)
            {
                const double threshold = std::midpoint(unique[k - 1], unique[k]);
                // заново распределяем всю выборку
                std::vector<double> left, right;
                for (std::size_t i = 0; i < x.rows(); ++i)
                {
                    if (x(i, j) <= threshold)
                    {
                        left.push_back(y[i]);
                    }
                    else
                    {
                        right.push_back(y[i]);
                    }
                }

                if (left.size() < min_leaf || right.size() < min_leaf)
                    continue;

                const double score = contribution(left, criterion) + contribution(right, criterion);

                if (!result.split || score > result.score)
                    result = {true, j, threshold, leaf_value(left, criterion),
                              leaf_value(right, criterion), score};
            }
        }
        return result;
    }

    void compare(const Matrix &x,
                 const std::vector<double> &y,
                 SplitCriterion criterion,
                 std::size_t min_leaf,
                 unsigned trial = 0)
    {
        const auto expected = naive_fit(x, y, criterion, min_leaf);

        DecisionStump model(criterion, min_leaf);
        model.fit(x, y);
        const auto predictions = model.predict(x);

        bool valid = model.has_split() == expected.split &&
                     close(model.left_value(), expected.left) &&
                     close(model.right_value(), expected.right);

        if (expected.split)
            valid = valid &&
                    model.split_feature() == expected.feature &&
                    close(model.split_threshold(), expected.threshold);

        assert(predictions.size() == y.size());

        for (std::size_t i = 0; i < x.rows(); ++i)
        {
            double value;

            if (!expected.split)
            {
                value = expected.left;
            }
            else if (x(i, expected.feature) <= expected.threshold)
            {
                value = expected.left;
            }
            else
            {
                value = expected.right;
            }

            valid = valid && close(predictions[i], value);
        }

        if (!valid)
        {
            std::cerr << std::setprecision(17)
                      << "Несовпадение: испытание=" << trial
                      << ", критерий=" << (criterion == SplitCriterion::Gini ? "Gini" : "Variance")
                      << ", min_samples_leaf=" << min_leaf << '\n'
                      << "Эталон: " << expected.split << ' ' << expected.feature << ' '
                      << expected.threshold << ' ' << expected.left << ' ' << expected.right
                      << ", качество=" << expected.score << '\n'
                      << "Модель: " << model.has_split() << ' ' << model.split_feature() << ' '
                      << model.split_threshold() << ' ' << model.left_value() << ' ' << model.right_value() << '\n';

            if (model.has_split())
            {
                std::vector<double> left, right;
                for (std::size_t i = 0; i < x.rows(); ++i)
                    (x(i, model.split_feature()) <= model.split_threshold() ? left : right).push_back(y[i]);

                if (!left.empty() && !right.empty())
                    std::cerr << "Качество модели: " << contribution(left, criterion) + contribution(right, criterion) << '\n';
            }

            for (std::size_t i = 0; i < x.rows(); ++i)
            {
                std::cerr << "Строка " << i << ": ";

                for (std::size_t j = 0; j < x.cols(); ++j)
                    std::cerr << x(i, j) << ' ';

                std::cerr << "target=" << y[i] << ", прогноз=" << predictions[i] << '\n';
            }
        }
        assert(valid);
    }

    void test_naive_comparison()
    {
        std::mt19937 generator(20261024);

        std::uniform_int_distribution<int> feature(-5, 5),
            regression(-20, 20),
            label(-2, 2);

        for (unsigned trial = 0; trial < 300; ++trial)
            for (auto criterion : {SplitCriterion::Variance, SplitCriterion::Gini})
            {
                const std::size_t n = 24 + generator() % 25, p = 1 + generator() % 5;

                Matrix x(n, p, trial % 2 ? Layout::ColumnMajor : Layout::RowMajor);
                std::vector<double> y(n);

                for (std::size_t i = 0; i < n; ++i)
                {
                    for (std::size_t j = 0; j < p; ++j)
                        x(i, j) = trial % 11 ? feature(generator) : 3;

                    if (trial % 13 == 0)
                    {
                        y[i] = -2;
                    }
                    else if (criterion == SplitCriterion::Gini)
                    {
                        y[i] = label(generator);
                    }
                    else
                    {
                        y[i] = regression(generator);
                    }
                }

                for (std::size_t min_leaf : {std::size_t{1}, std::size_t{3}, n / 2, n + 1})
                    compare(x, y, criterion, min_leaf, trial);
            }
    }

    void test_separable_data()
    {
        for (auto criterion : {SplitCriterion::Variance, SplitCriterion::Gini})
        {
            Matrix x(6, 2, Layout::RowMajor);
            const std::vector<double> y{-3, -3, -3, 7, 7, 7};

            for (std::size_t i = 0; i < 6; ++i)
            {
                x(i, 0) = i % 3;
                x(i, 1) = i;
            }

            DecisionStump model(criterion, 1);
            model.fit(x, y);

            assert(model.has_split() && model.split_feature() == 1);
            assert(close(model.split_threshold(), 2.5));
            assert(model.predict(x) == y);

            Matrix probes(3, 2, Layout::ColumnMajor);
            probes(0, 1) = 2.5;
            probes(1, 1) = std::nextafter(2.5, 3.0);
            probes(2, 1) = -100;

            assert((model.predict(probes) == std::vector<double>{-3, 7, -3}));
            compare(x, y, criterion, 1);
        }
    }

    void test_constant_feature()
    {
        Matrix x(4, 2, Layout::ColumnMajor);
        const std::vector<double> y{0, 0, 1, 1};

        for (std::size_t i = 0; i < 4; ++i)
        {
            x(i, 0) = 10;
            x(i, 1) = i;
        }

        for (auto criterion : {SplitCriterion::Variance, SplitCriterion::Gini})
        {
            DecisionStump model(criterion, 1);
            model.fit(x, y);

            assert(model.has_split() && model.split_feature() == 1);
            compare(x, y, criterion, 1);
        }
    }

    void test_duplicate_values()
    {
        Matrix x(6, 1, Layout::RowMajor);
        const std::vector<double> y{-2, -2, -2, 4, 4, 4};

        for (std::size_t i = 0; i < 6; ++i)
            x(i, 0) = i < 3 ? 1 : 5;

        for (auto criterion : {SplitCriterion::Variance, SplitCriterion::Gini})
        {
            DecisionStump model(criterion, 3);
            model.fit(x, y);

            assert(model.has_split() && close(model.split_threshold(), 3));
            assert(model.predict(x) == y);
            compare(x, y, criterion, 3);

            model = DecisionStump(criterion, 4);
            model.fit(x, y);

            assert(!model.has_split());
            compare(x, y, criterion, 4);

            Matrix constant(6, 1, Layout::RowMajor);
            model.fit(constant, y);

            assert(!model.has_split());

            double expected;

            if (criterion == SplitCriterion::Gini)
            {
                expected = -2;
            }
            else
            {
                expected = 1;
            }

            assert(model.predict(constant) == std::vector<double>(6, expected));
            compare(constant, y, criterion, 1);
        }
    }

    void test_imodel_integration()
    {
        Matrix x(4, 1, Layout::RowMajor);

        for (std::size_t i = 0; i < 4; ++i)
            x(i, 0) = i;

        const std::vector<double> y{2, 2, 8, 8};
        for (auto criterion : {SplitCriterion::Variance, SplitCriterion::Gini})
        {
            std::unique_ptr<IModel> model = std::make_unique<DecisionStump>(criterion, 1);
            model->fit(x, y);

            assert(model->predict(x) == y);
        }
    }

    template <typename Exception, typename Action>
    void expect_exception(Action action)
    {
        bool caught = false;
        try
        {
            action();
        }
        catch (const Exception &)
        {
            caught = true;
        }
        assert(caught);
    }

    void test_decision_stump_exceptions()
    {
        for (auto criterion : {SplitCriterion::Variance, SplitCriterion::Gini})
        {
            expect_exception<std::invalid_argument>([&]
                                                    { DecisionStump model(criterion, 0); });

            DecisionStump model(criterion, 1);
            Matrix x(2, 1, Layout::RowMajor);
            const std::vector<double> y{1, 2};

            expect_exception<std::logic_error>([&]
                                               { model.predict(x); });
            expect_exception<std::invalid_argument>([&]
                                                    { model.fit(Matrix(0, 1, Layout::RowMajor), {}); });
            expect_exception<std::invalid_argument>([&]
                                                    { model.fit(Matrix(2, 0, Layout::RowMajor), y); });
            expect_exception<std::invalid_argument>([&]
                                                    { model.fit(x, std::vector<double>{1}); });
            expect_exception<std::invalid_argument>([&]
                                                    { model.fit(x, std::vector<double>{1, 2, 3}); });

            model.fit(x, y);

            for (std::size_t cols : {std::size_t{0}, std::size_t{2}})
                expect_exception<std::invalid_argument>([&]
                                                        { model.predict(Matrix(2, cols, Layout::RowMajor)); });

            assert(model.predict(Matrix(0, 1, Layout::RowMajor)).empty());
        }
    }
} // namespace

int main()
{
    test_naive_comparison();
    test_separable_data();
    test_constant_feature();
    test_duplicate_values();
    test_imodel_integration();
    test_decision_stump_exceptions();
    std::cout << "Тесты DecisionStump пройдены\n";
}
