#pragma once

#include "search_server.h"

#include <string>
#include <vector>

void MatchDocuments(const SearchServer& search_server, const std::string& query);

void FindTopDocuments(const SearchServer& search_server, const std::string& query);

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);