#include "test_example_functions.h"

#include "log_duration.h"

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
    std::cout << "Матчинг документов по запросу: " << query << std::endl;
    LOG_DURATION_STREAM("Operation time", std::cout);

    for (const int doc_id : search_server) {
        const auto [v_words, status] = search_server.MatchDocument(query, doc_id);
        std::cout << "{ document_id = " << doc_id << ", status = " << static_cast<int>(status) << ", words = " << v_words << "}" << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& query) {
    std::cout << "Результаты поиска по запросу: " << query << std::endl;
    LOG_DURATION_STREAM("Operation time", std::cout);
    std::vector<Document> v_result = search_server.FindTopDocuments("curly -cat");
    for (const auto& elem : v_result) {
        std::cout << "{ document_id = " << elem.id << ", relevance = " << elem.relevance << ", rating = " << elem.rating << " }" << std::endl;
    }
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}