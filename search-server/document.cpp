#include "document.h"

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating)
{}

std::ostream& operator<<(std::ostream& os, const Document& doc) {
    return os << "{ document_id = " << doc.id << ", relevance = " << doc.relevance << ", rating = " << doc.rating << " }";
}

std::ostream& operator<<(std::ostream& os, const std::vector<std::string_view>& words) {
    for (const auto& word : words) {
        os << word << " ";
    }
    return os;
}
