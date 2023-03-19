#pragma once

#include <string>
#include <ostream>

#include "document.h"
#include "paginator.h"

std::string ReadLine();
int ReadLineWithNumber();

std::ostream& operator<<(std::ostream& os, const Document& doc);

template <typename Iterator>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iterator>& range) {
    for (auto iter = range.begin(); iter != range.end(); ++iter) {
        os << *iter;
    }
    return os;
}