#include "process_queries.h"

#include <numeric>
#include <execution>

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> v_results(queries.size());

    std::transform(
        std::execution::par,
        queries.begin(), queries.end(),
        v_results.begin(),
        [&search_server](const std::string& query) { return search_server.FindTopDocuments(query); }
    );

    return v_results;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> v_v_documents = ProcessQueries(search_server, queries);
    std::vector<Document> v_result;

    for (const auto doc : v_v_documents) {
        v_result.insert(v_result.end(), doc.begin(), doc.end());
    }

    return v_result;
}