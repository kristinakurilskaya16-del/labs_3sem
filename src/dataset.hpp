#include "types.hpp"
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <unordered_map>
#include <filesystem>

class Dataset
{
private:
    std::unordered_map<std::string, Column> columns_;

public:
    void add_column(const std::string &name, const Column &col)
    {
        columns_[name] = col;
    }

    const std::unordered_map<std::string, Column> &get_columns() const
    {
        return columns_;
    }

    std::size_t get_num_rows() const;

    void print_summary(std::ostream &out = std::cout) const;

    DatasetInfo analyze() const;

    std::vector<std::string> get_feature_names() const;
};

std::vector<std::string> split(
    const std::string &line,
    char delimiter = ',');

Dataset load_dataset(
    const std::filesystem::path &filepath,
    char delimiter = ',',
    const std::string &missing_marker = "NA");

double quantile(const std::vector<double> &sorted_values, double p);

void print_numeric_histogram(const std::vector<double> &values, double min, double max,
                             int num_bins = 10);
void print_categorical_histogram(const CategoricalFeatureInfo &cat_info);

void print_numeric_info(std::size_t index, const std::string &name, const NumericFeatureInfo &info,
                        std::span<const double> clean_values);
void print_categorical_info(std::size_t index, const std::string &name, const CategoricalFeatureInfo &info);
