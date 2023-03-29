#pragma once

#include <ostream>
#include <vector>

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating);

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

std::ostream& operator<<(std::ostream& os, const Document& doc);
std::ostream& operator<<(std::ostream& os, const std::vector<std::string>& words);