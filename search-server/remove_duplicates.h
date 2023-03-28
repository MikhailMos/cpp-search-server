#pragma once

#include "search_server.h"

std::set<std::string> ConvertMapToSet(const std::map<std::string, double>& map_words);

void RemoveDuplicates(SearchServer& search_server);
