#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6; // погрешность

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string &text)
{
    vector<string> words;
    string word;
    for (const char c : text)
    {
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }

    return words;
}

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:

    SearchServer() = default;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (const string& word : stop_words) {
            if (!IsValidWord(word)) { throw invalid_argument("The stop-words contain invalid characters"s); }
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {

        if (document_id < 0) {
            throw invalid_argument("Id of a document cannot be lower than zero"s);
        }

        if (documents_.count(document_id) != 0) {
            throw invalid_argument("Document with this id is already exists"s);
        }

        const vector<string> words = SplitIntoWordsNoStop(document);
        // проверим содержание документа на спецсимволы
        for (const string& word : words) {
            if (!IsValidWord(word)) {
                throw invalid_argument("The document text contains invalid characters"s);
            }
        }

        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        ids_of_documents_.push_back(document_id);
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {

        const Query query = ParseQuery(raw_query);

        vector<Document> result = FindAllDocuments(query, document_predicate);

        sort(result.begin(), result.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {

        const Query query = ParseQuery(raw_query);

        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }

        return tuple{ matched_words, documents_.at(document_id).status };
    }

    int GetDocumentId(int index) const {
        
        if (index < 0 || index > static_cast<int>(documents_.size())) {
            throw out_of_range("Index out of range"s);
        }
        
        return ids_of_documents_.at(index);
        
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_ = {};
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> ids_of_documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);

            if (!IsValidWord(word)) {
                throw invalid_argument("The query text contains invalid characters"s);
            }
            if (query_word.data[0] == '-') {
                throw invalid_argument("The query text contains word which has more than one minus before it"s);
            }
            if (query_word.data.size() == 0) {
                throw invalid_argument("The query text hasn't word after character minus"s);
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
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
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

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

/*
   Макросы ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T>
void RunTestImpl(const T&, const string& t_str) {
    cerr << t_str << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        ASSERT(server.FindTopDocuments("in"s).empty());
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

// Тест проверяет, что поисковая система исключает минус-слова из поискового запроса
void TestExcludeMinusWordsFromQuery() {
    
    const int doc_id1 = 42;
    const int doc_id2 = 24;    
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = { -1, 2, 2 };
    const string content_2 = "dog of a hidden village"s;
    const vector<int> ratings_2 = { 1, 2, 3 };
    // сначала убедимся, что слова не являющиеся минус-словами, находят нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id1);
    }
    // затем убедимся, что поиск по минус-слову возвращает второй документ (id = 24) 
    {        
        SearchServer server;
        server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("-in the dog"s);
        ASSERT(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id2);
    }
}

// Тест на проверку сопоставления содержимого документа и поискового запроса
void TestMatchDocument() {

    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    // убедимся,что слова из поискового запроса вернулись
    {
        const auto found_tup = server.MatchDocument("in the cat"s, doc_id);        
        const auto& [words, status_doc] = found_tup;
        ASSERT_EQUAL(words.size(), 3);
    }

    // убедимся, что присутствие минус-слова возвращает пустой вектор
    {
        const auto found_tup = server.MatchDocument("in -the cat"s, doc_id);
        const auto& [words, status_doc] = found_tup;
        ASSERT_EQUAL(words.empty(), true);
    }

}

// Тест на сортировку по релевантности найденых документов
void TestByRelevance() {
    
    const double epx = 1e-6; //+-0.000001
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = { -1, 2, 2 }; // rating 1 relev = 0.304098831081123
    const string content_2 = "dog of a hidden village"s;
    const vector<int> ratings_2 = { 1, 2, 3 }; // rating 2 relev = 0.0810930216216328
    const string content_3 = "silent assasin village cat in the village of darkest realms"s;
    const vector<int> ratings_3 = { 2, 3, 4 }; // rating 3 relev = 0.202732554054082

    SearchServer server;
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::ACTUAL, ratings_3);

    // убедимся,что документы найдены
    const auto found_docs = server.FindTopDocuments("cat in the loan village"s);
    ASSERT_EQUAL(found_docs.size(), 3);

    const Document& doc1 = found_docs[0];
    const Document& doc2 = found_docs[1];
    const Document& doc3 = found_docs[2];

    // проверим сортировку по релевантности
    ASSERT(doc1.relevance > doc2.relevance > doc3.relevance);
    
    // значение релевантности вычисляется по формуле
    // idf = log(server.GetDocumentCount() * 1.0 / 1), где числитель - это кол-во документов, знаменатель это кол-во документов, где встречается слово запроса. 
    // PS: НЕ встречающиеся ни где слова в расчет не берутся!
    // tf = (а / b), где а - сколько раз встречается слово в документе, b - кол-во слов в документе.    
    // затем произведения tf и idf в документе суммируются
    
    double relev1 = log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 4) 
                    + log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 4) 
                    + log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 4) 
                    + 0 
                    + log(server.GetDocumentCount() * 1.0 / 2) * (0.0 / 4);

    double relev2 = log(server.GetDocumentCount() * 1.0 / 2) * (0.0 / 5) 
                    + log(server.GetDocumentCount() * 1.0 / 2) * (0.0 / 5) 
                    + log(server.GetDocumentCount() * 1.0 / 2) * (0.0 / 5) 
                    + 0 
                    + log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 5);

    double relev3 = log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 4) 
                    + log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 4) 
                    + log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 4) 
                    + 0 
                    + log(server.GetDocumentCount() * 1.0 / 2) * (0.0 / 4);
            
    // проверим на точность вычисления значений в рамках погрешности    
    ASSERT(abs(doc1.relevance - relev1) < epx);
    ASSERT(abs(doc2.relevance - relev3) < epx);
    ASSERT(abs(doc3.relevance - relev2) < epx);

}

// Тест по рейтингу
void TestByRating() {
    
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = { -1, 2, 2 }; // rating 1 relev = 0.304098831081123
    const string content_2 = "dog of a hidden village"s;
    const vector<int> ratings_2 = { 1, 2, 3 }; // rating 2 relev = 0.0810930216216328
    const string content_3 = "silent assasin village cat in the village of darkest realms"s;
    const vector<int> ratings_3 = { 2, 3, 4 }; // rating 3 relev = 0.202732554054082

    SearchServer server;
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::ACTUAL, ratings_3);

    // убедимся,что документы найдены
    const auto found_docs = server.FindTopDocuments("cat in the loan village"s);
    ASSERT_EQUAL(found_docs.size(), 3);

    const Document& doc1 = found_docs[0];
    const Document& doc2 = found_docs[1];
    const Document& doc3 = found_docs[2];

    // проверим рейтинг
    ASSERT_EQUAL(doc1.rating, (-1 + 2 + 2) / 3);
    ASSERT_EQUAL(doc2.rating, (2 + 3 + 4) / 3);
    ASSERT_EQUAL(doc3.rating, (1 + 2 + 3) / 3);

}

// Тест на фильтрацию результата с использованием предиката
void TestFilterByPredicate() {
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = { -1, 2, 2 }; // rating 1 relev = 0.304098831081123
    const string content_2 = "dog of a hidden village"s;
    const vector<int> ratings_2 = { 1, 2, 3 }; // rating 2 relev = 0.0810930216216328
    const string content_3 = "silent assasin village cat in the village of darkest realms"s;
    const vector<int> ratings_3 = { 2, 3, 4 }; // rating 3 relev = 0.202732554054082

    SearchServer server;
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::BANNED, ratings_3);

    // веренем документы с не четным ИД
    {
        const auto found_docs = server.FindTopDocuments("cat in the loan village"s,
            [](int document_id, DocumentStatus, int)
            {
                return document_id % 2 != 0;
            });

        ASSERT_EQUAL(found_docs.size(), 2);
    }

    // вернем документы с рейтингом
    {
        const auto found_docs = server.FindTopDocuments("cat in the loan village"s,
            [](int, DocumentStatus, int rating)
            {
                return rating == 3;
            });

        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc1 = found_docs[0];
        ASSERT_EQUAL(doc1.id, doc_id3);

    }

}

void TestSearchByStatusDocuments() {
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const int doc_id4 = 4;
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = { -1, 2, 2 };
    const string content_2 = "dog of a hidden village"s;
    const vector<int> ratings_2 = { 1, 2, 3 };
    const string content_3 = "silent assasin village cat in the village of darkest realms"s;
    const vector<int> ratings_3 = { 2, 3, 4 };
    const string content_4 = "cat of the loan village"s;
    const vector<int> ratings_4 = { 6, 4, 2 };

    SearchServer server;
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::IRRELEVANT, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::BANNED, ratings_3);
    server.AddDocument(doc_id4, content_4, DocumentStatus::REMOVED, ratings_4);

    // вернем документы со статусом Актуальный
    {
        const auto found_docs = server.FindTopDocuments("cat of village"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs[0].id, doc_id1);
    }

    // вернем документы со статусом Нерелевантынй
    {
        const auto found_docs = server.FindTopDocuments("cat of village"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found_docs[0].id, doc_id2);
    }

    // вернем документы со статусом Заблокированный
    {
        const auto found_docs = server.FindTopDocuments("cat of village"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs[0].id, doc_id3);
    }

    // вернем документы со статусом Удаленный
    {
        const auto found_docs = server.FindTopDocuments("cat of village"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(found_docs[0].id, doc_id4);
    }

}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWordsFromQuery);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestByRelevance);
    RUN_TEST(TestByRating);
    RUN_TEST(TestFilterByPredicate);
    RUN_TEST(TestSearchByStatusDocuments);
}

// --------- Окончание модульных тестов поисковой системы -----------

// ==================== для примера =========================

void PrintDocument(const Document &document)
{
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}
int main()
{
    
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
    
    SearchServer search_server("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document &document : search_server.FindTopDocuments("пушистый ухоженный кот"s))
    {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document &document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED))
    {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document &document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating)
                                                                   { return document_id % 2 == 0; }))
    {
        PrintDocument(document);
    }

    // --------- Проверки --------
    // проверка не допустимых символов в стоп-словах
    try {
        SearchServer search_server1("и в на скво\x12рец"s);
    }
    catch (const invalid_argument& e) {
        cout << "Error: "s << e.what() << endl;
    }

    try {
        // проверка на попытку добавить документ с уже имеющимя ид в системе
        search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        //// проверка на добавление документа с отрицательным ид
        //search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        //// попытка добавить документ с текстом содержащим недопустимые символы
        //search_server.AddDocument(4, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
        //// проверка на содержание нескольких минусов
        //const auto documents = search_server.FindTopDocuments("--пушистый"s);
        //for (const Document& document : documents) {
        //    PrintDocument(document);
        //}
    }
    catch (const invalid_argument& e) {
        cout << "Error: "s << e.what() << endl;
    }

    // проверка выхода индекса за допустимые пределы
    try {
        const int id = search_server.GetDocumentId(100);
        cout << "doc id: "s << id << endl;
    }
    catch (const out_of_range& e) {
        cout << "Error: "s << e.what() << endl;
    }

    return 0;
}