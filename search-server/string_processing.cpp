#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(const std::string_view& text)
{
        
    std::vector<std::string_view> words;
    words.reserve((text.length() / 2));

    size_t start_position{ text.find_first_not_of(' ') }; // начальный индекс первого слова
    while (start_position != std::string_view::npos) {
        size_t end_position = text.find_first_of(' ', start_position + 1);
        if (end_position == std::string_view::npos) {
            end_position = text.length();
        }

        words.push_back(text.substr(start_position, (end_position - start_position)));
        start_position = text.find_first_not_of(' ', end_position + 1);

    }

    return words;
}