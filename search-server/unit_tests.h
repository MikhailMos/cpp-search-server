#pragma once

#include "macros.h"

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent();

// Тест проверяет, что поисковая система исключает минус-слова из поискового запроса
void TestExcludeMinusWordsFromQuery();

// Тест на проверку сопоставления содержимого документа и поискового запроса
void TestMatchDocument();

// Тест на сортировку по релевантности найденых документов
void TestByRelevance();

// Тест по рейтингу
void TestByRating();

// Тест на фильтрацию результата с использованием предиката
void TestFilterByPredicate();

void TestSearchByStatusDocuments();

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();