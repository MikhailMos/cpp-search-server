#include "read_input_functions.h"

#include <iostream>

std::string ReadLine()
{
    std::string s;
    std::getline(std::cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}

std::ostream& operator<<(std::ostream& os, const Document& doc) {
    return os << "{ document_id = " << doc.id << ", relevance = " << doc.relevance << ", rating = " << doc.rating << " }";
}
