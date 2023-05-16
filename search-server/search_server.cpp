#include "search_server.h"

#include <cmath>
#include <numeric>
#include <iterator>


SearchServer::SearchServer(const std::string_view& stop_words_text)
    : SearchServer::SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{}

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer::SearchServer(std::string_view{ stop_words_text })
{}

void SearchServer::AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings) {

    if (document_id < 0) {
        throw std::invalid_argument("Id of a document cannot be lower than zero");
    }

    if (documents_.count(document_id) != 0) {
        throw std::invalid_argument("Document with this id is already exists");
    }

    const std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    // проверим содержание документа на спецсимволы
    for (const std::string_view& word : words) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("The document text contains invalid characters");
        }
    }

    std::map<std::string_view, double> words_freqs;

    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view& word : words) {
        const auto curr_elem = buffer_.emplace(std::string{ word }); // pair(iterator, bool)
        word_to_document_freqs_[*(curr_elem.first)][document_id] += inv_word_count;
        words_freqs[*(curr_elem.first)] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    ids_of_documents_.insert(document_id);
    words_with_frequency_by_doc_id_.emplace(document_id, words_freqs);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

 std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query, int document_id) const {

    const Query query = ParseQuery(raw_query);
        
    std::vector<std::string_view> matched_words;
    
    for (const std::string_view& word : query.minus_words) {
        
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return std::tuple{ matched_words, documents_.at(document_id).status };
        }
        
    }
    
    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    
    return std::tuple{ matched_words, documents_.at(document_id).status };
 }

 std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy policy, const std::string_view& raw_query, int document_id) const {

     return SearchServer::MatchDocument(raw_query, document_id);

 }

 std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy, const std::string_view& raw_query, int document_id) const {
     
     const Query query = ParseQuery(raw_query, false);

     const std::map<std::string_view, double>& word_freq_in_doc = words_with_frequency_by_doc_id_.at(document_id);

     if (std::any_of(policy,
         query.minus_words.begin(), query.minus_words.end(),
         [&word_freq_in_doc](const std::string_view& word) {
             return !(word_freq_in_doc.count(word) == 0);
         })) 
     {

         return std::tuple{ std::vector<std::string_view>{}, documents_.at(document_id).status };
     }

     std::vector<std::string_view> matched_words(query.plus_words.size());

     const auto last_it = std::copy_if(policy,
        query.plus_words.begin(),
        query.plus_words.end(),
        matched_words.begin(),
        [&word_freq_in_doc](const std::string_view& word) {
            return !(word_freq_in_doc.count(word) == 0);
        });

     matched_words.resize(std::distance(matched_words.begin(), last_it));

     RemoveDublicatesFromVector(matched_words);
     
     return std::tuple{ matched_words, documents_.at(document_id).status };

 }

 std::set<int>::const_iterator SearchServer::begin() const {
     return ids_of_documents_.begin();
 }

 std::set<int>::const_iterator SearchServer::end() const {
     return ids_of_documents_.end();
 }

 const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
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

             // если слово не находится ни в одном из документов, удалим его
             if (0 == it->second.size()) {
                 EraseWordFromBuffer(it->first);
                 it = word_to_document_freqs_.erase(it);
             }
             else {
                 ++it;
             }

         }
     }
     
 }

 void SearchServer::RemoveDocument(std::execution::sequenced_policy policy, int document_id) {
     SearchServer::RemoveDocument(document_id);
 }

 void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id) {
     
     {
         auto iter = words_with_frequency_by_doc_id_.find(document_id);

         if (iter == words_with_frequency_by_doc_id_.end()) { return; }
         else {             
             std::vector<const std::string_view*> words_for_erase(iter->second.size());
                          
             std::transform(policy,
                 iter->second.begin(),
                 iter->second.end(),
                 words_for_erase.begin(),
                 [](auto& word_freq) { return &(word_freq).first; } //&const_cast<std::string&>(word_freq.first);
             );
             
             std::for_each(policy, 
                           words_for_erase.begin(), 
                           words_for_erase.end(), 
                           [&](auto& word) {
                                                                
                                auto& elem = word_to_document_freqs_.at(*word);
                                elem.erase(document_id);
                                // если слово не содержится ни в одном документе, удалим его из буффера
                                if (0 == elem.size()) {
                                    EraseWordFromBuffer(*word);
                                }
                           }
             );
             
             words_with_frequency_by_doc_id_.erase(iter);
         }
     }

     {
         auto iter = documents_.find(document_id);

         if (iter == documents_.end()) { return; }
         else {
             documents_.erase(iter);
         }
     }

     {        
         ids_of_documents_.erase(std::find(policy, ids_of_documents_.begin(), ids_of_documents_.end(), document_id));
     }

 }
  
// private
bool SearchServer::IsStopWord(const std::string_view& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
    std::vector<std::string_view> words;
    for (const std::string_view& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

bool SearchServer::IsValidWord(const std::string_view& word) {
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text, const bool is_remove_duplicates) const {
    SearchServer::Query query;
    for (const std::string_view& word : SplitIntoWords(text)) {
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
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }

    // избавимся от дублей
    if (is_remove_duplicates) {
        RemoveDublicatesFromVector(query.minus_words);
        RemoveDublicatesFromVector(query.plus_words);
    }

    return query;
}

void SearchServer::RemoveDublicatesFromVector(std::vector<std::string_view>& v_words) const {
    std::sort(v_words.begin(), v_words.end());
    v_words.erase(std::unique(v_words.begin(), v_words.end()), v_words.end());
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void SearchServer::EraseWordFromBuffer(std::string_view sv_word) {
    const auto it_word = buffer_.find(std::string{ sv_word });
    if (it_word != buffer_.end()) {
        buffer_.erase(it_word);
    }
}