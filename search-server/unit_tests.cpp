#include "unit_tests.h"

#include "search_server.h"

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