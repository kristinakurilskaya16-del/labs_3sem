#pragma once

#include "imodel.hpp"

#include <cstddef>
#include <span>
#include <vector>
#include <stdexcept>

enum class SplitCriterion
{
    Variance,
    Gini
};

class DecisionStump : public IModel
{
private:
    std::size_t feature_index_ = 0;
    double threshold_ = 0.0;

    double left_value_ = 0.0;
    double right_value_ = 0.0;

    SplitCriterion criterion_;
    std::size_t min_samples_leaf_;

    std::size_t n_features_ = 0;
    bool is_fitted_ = false;
    bool has_split_ = false;

public:
    DecisionStump(SplitCriterion criterion, std::size_t min_samples_leaf)
        : criterion_(criterion),
          min_samples_leaf_(min_samples_leaf)
    {
        if (min_samples_leaf_ == 0)
            throw std::invalid_argument("min_samples_leaf должен быть больше нуля");
    }

    void fit(const Matrix &features, std::span<const double> target) override;
    std::vector<double> predict(const Matrix &features) const override;

    std::size_t split_feature() const { return feature_index_; }
    double split_threshold() const { return threshold_; }
    bool has_split() const { return has_split_; }

    double left_value() const { return left_value_; }
    double right_value() const { return right_value_; }
};