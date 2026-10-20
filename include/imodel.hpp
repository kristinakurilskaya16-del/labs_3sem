#pragma once

#include "matrix.hpp"
#include <vector>
#include <span>

class IModel
{
public:
    virtual ~IModel() = default;

    virtual void fit(
        const Matrix &features,
        std::span<const double> target) = 0;

    virtual std::vector<double> predict(
        const Matrix &features) const = 0;
};