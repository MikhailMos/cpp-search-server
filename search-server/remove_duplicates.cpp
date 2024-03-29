#include "remove_duplicates.h"

#include <iostream>
#include <set>
#include <string>
#include <string_view>

std::set<std::string_view> ConvertMapToSet(const std::map<std::string_view, double>& map_words) {
    std::set<std::string_view> result;

    for (auto it = map_words.begin(); it != map_words.end(); ++it) {
        result.insert(it->first);
    }

    return result;
}

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> id_for_remove;
    std::set<std::set<std::string_view>> unique_words;

    for (const int document_id : search_server) {
        const std::map<std::string_view, double>& map_words = search_server.GetWordFrequencies(document_id);
        const std::set<std::string_view> set_words = ConvertMapToSet(map_words);

        if (unique_words.count(set_words) != 0) {
            id_for_remove.insert(document_id);
        }
        else {
            unique_words.insert(set_words);
        }
    }
    
    for (const int id : id_for_remove) {
        search_server.RemoveDocument(id);
        std::cout << "Found duplicate document id " << id << std::endl;
    }
    
}