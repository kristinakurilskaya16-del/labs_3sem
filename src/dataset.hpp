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
    std::size_t num_rows_ = 0;

public:
    void add_column(const std::string &name, const Column &col)
    {
        columns_[name] = col;
    }

    const std::unordered_map<std::string, Column> &get_columns() const
    {
        return columns_;
    }

    void set_num_rows(std::size_t count)
    {
        num_rows_ = count;
    }

    void print_summary(std::ostream &out = std::cout) const;
};

std::vector<std::string> split(
    const std::string &line,
    char delimiter = ',');

Dataset load_dataset(
    const std::filesystem::path &filepath,
    char delimiter = ',', // разделитель
    const std::string &missing_marker = "NA");