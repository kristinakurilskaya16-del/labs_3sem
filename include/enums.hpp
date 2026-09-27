#pragma once

enum class NumericImputation
{
    Mean,
    Median,
    Zero
};

enum class Scaling
{
    MinMax,
    Standard,
    Robust
};

enum class Encoding
{
    OneHot,
    Ordinal
};

enum class Layout
{
    RowMajor,
    ColumnMajor
};