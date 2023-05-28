#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <stdexcept>
#include <algorithm>
#include <typeinfo>
#include <execution>
#include <string_view>

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

class SearchServer {   
public:

    SearchServer() = default;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string_view& stop_words_text);
    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings);

    //FindTopDocuments с 3 параметром без учета политики
    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const;

    //FindTopDocuments с 2 параметром без учета политики
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const;

    //FindTopDocuments с 1 параметром без учета политики
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query) const;
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view& raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy policy, const std::string_view& raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy policy, const std::string_view& raw_query, int document_id) const;

    std::set<int>::const_iterator begin() const;

    std::set<int>::const_iterator end() const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    void RemoveDocument(std::execution::sequenced_policy policy, int document_id);

    void RemoveDocument(std::execution::parallel_policy policy, int document_id);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;        
    };
    const std::set<std::string, std::less<>> stop_words_ = {}; // стоп-слова
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_; // слова содержащиеяся в документах и их частота в этом документе {<слово> {{<ид_документа>, <частота>},...}} 
    std::map<int, DocumentData> documents_; // {<ид_док>, {<рейтинг>, <статус>}}
    std::set<int> ids_of_documents_; // все ИД добавленых документов
    std::map<int, std::map<std::string_view, double>> words_with_frequency_by_doc_id_; // {doc_id {word, freq}}
    const std::map<std::string_view, double> EMPTY_MAP_WORDS_FREQS_;
    std::set<std::string, std::less<>> buffer_ = {}; // содержит слова, string_view данных будет ссылаться на него

    bool IsStopWord(const std::string_view& word) const;

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view& text) const;

    static bool IsValidWord(const std::string_view& word);

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
        
    Query ParseQuery(const std::string_view& text, const bool is_remove_duplicates = true) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy& policy, const Query& query,
        DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy& policy, const Query& query,
        DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const;

    void RemoveDublicatesFromVector(std::vector<std::string_view>& v_words) const;

    void EraseWordFromBuffer(std::string_view sv_word);
   
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    for (const std::string_view& word : stop_words) {
        if (!IsValidWord(word)) { throw std::invalid_argument("The stop-words contain invalid characters"); }
    }
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query, DocumentPredicate document_predicate) const {
    const int MAX_RESULT_DOCUMENT_COUNT = 5;
    const double EPSILON = 1e-6; // погрешность

    const Query query = ParseQuery(raw_query);

    std::vector<Document> result = FindAllDocuments(policy, query, document_predicate);

    std::sort(policy,
        result.begin(), result.end(),
        [&EPSILON](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
        result.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return result;   
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const {    
    return SearchServer::FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query) const {
    return SearchServer::FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy& policy, const Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(100);  //100 - наиболее оптимальное кол-во "корзин" для  распараллеливания
    
    std::for_each(policy,
        query.plus_words.begin(), query.plus_words.end(),
        [&](const std::string_view& word) {
            if (word_to_document_freqs_.count(word) != 0) {

                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                        document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
            }
        }
    );

    std::for_each(policy,
        query.minus_words.begin(), query.minus_words.end(),
        [&](const std::string_view& word) {
            if (word_to_document_freqs_.count(word) != 0) {

                for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                    document_to_relevance.Erase(document_id);
                }
            }
        }
    );
        
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy& policy, const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    return SearchServer::FindAllDocuments(std::execution::seq, query, document_predicate);
}
