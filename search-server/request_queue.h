#pragma once

#include <deque>

#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        QueryResult() = default;

        std::string query = "";
        int count_docs = 0;
        int time = 0;
    };
    std::deque<QueryResult> requests_;
    const SearchServer& search_server_;
    int count_requests_without_result;
    int curr_time_;
    const static int min_in_day_ = 1440;

    void AddRequest(const std::string& raw_query, int count_of_docs);

};