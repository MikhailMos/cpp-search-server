#include "search_server.h"

#include <cmath>
#include <numeric>


SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer::SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {

    if (document_id < 0) {
        throw std::invalid_argument("Id of a document cannot be lower than zero");
    }

    if (documents_.count(document_id) != 0) {
        throw std::invalid_argument("Document with this id is already exists");
    }

    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    // �������� ���������� ��������� �� �����������
    for (const std::string& word : words) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("The document text contains invalid characters");
        }
    }

    std::map<std::string, double> words_freqs;

    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        words_freqs[word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    ids_of_documents_.push_back(document_id);
    words_with_frequency_by_doc_id_.emplace(document_id, words_freqs);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

 std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string & raw_query, int document_id) const {

        const Query query = ParseQuery(raw_query);

        std::vector<std::string> matched_words;
        for (const std::string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const std::string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }

        return std::tuple{ matched_words, documents_.at(document_id).status };
    }

 std::vector<int>::const_iterator SearchServer::begin() const {
     return ids_of_documents_.begin();
 }

 std::vector<int>::const_iterator SearchServer::end() const {
     return ids_of_documents_.end();
 }

 const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
     const auto iter = words_with_frequency_by_doc_id_.find(document_id);
     if (iter == words_with_frequency_by_doc_id_.end()) { return EMPTY_MAP_WORDS_FREQS_; }

     return iter->second;
 }

 void SearchServer::RemoveDocument(int document_id) {
     {
         auto iter = words_with_frequency_by_doc_id_.find(document_id);

         if (iter != words_with_frequency_by_doc_id_.end()) {
             words_with_frequency_by_doc_id_.erase(iter);
         }
     }

     {
         auto iter = documents_.find(document_id);

         if (iter != documents_.end()) {
             documents_.erase(iter);
         }
     }

     {
         for (auto iter = ids_of_documents_.begin(); iter != ids_of_documents_.end(); ++iter) {
            if (*iter == document_id) {
                ids_of_documents_.erase(iter);
                break;
            }
         }
     }

     {
         for (auto it = word_to_document_freqs_.begin(); it != word_to_document_freqs_.end();) {
                          
             auto iter = it->second.find(document_id);
             if (iter != it->second.end()) {
                 it->second.erase(iter);
             }

             if (0 == it->second.size()) {
                 it = word_to_document_freqs_.erase(it);
             }
             else {
                 ++it;
             }

         }
     }
     
 }
  
// private
bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

bool SearchServer::IsValidWord(const std::string& word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);

    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    SearchServer::Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);

        if (!IsValidWord(word)) {
            throw std::invalid_argument("The query text contains invalid characters");
        }
        if (query_word.data[0] == '-') {
            throw std::invalid_argument("The query text contains word which has more than one minus before it");
        }
        if (query_word.data.size() == 0) {
            throw std::invalid_argument("The query text hasn't word after character minus");
        }

        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            }
            else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}