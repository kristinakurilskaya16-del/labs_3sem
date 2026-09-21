#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <unordered_map>
#include "dataset.hpp"

int main()
{
    Dataset dataset = load_dataset("data/data.csv");
    dataset.print_summary();

    DatasetInfo info = dataset.analyze();

    int number;
    while (true)
    {
        std::cout << "\nВведите номер признака(или 'stop' для выхода): ";
        std::string input;
        std::getline(std::cin, input);

        if (input.empty() && input.back() == '\r')
        {
            input.pop_back();
        }

        if (input == "stop")
        {
            break;
        }
    }

    return 0;
}