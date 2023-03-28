#include "remove_duplicates.h"

#include <iostream>
#include <set>
#include <string>

std::set<std::string> ConvertMapToSet(const std::map<std::string, double>& map_words) {
    std::set<std::string> result;

    for (auto it = map_words.begin(); it != map_words.end(); ++it) {
        result.insert(it->first);
    }

    return result;
}

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> id_for_remove;

    for (auto it1 = search_server.begin(); it1 != search_server.end(); ++it1) {
        const std::map<std::string, double>& map_words = search_server.GetWordFrequencies(*it1);
        const std::set<std::string> set_words1 = ConvertMapToSet(map_words);

        for (auto it2 = (it1 + 1); it2 != search_server.end(); ++it2) {
            const std::map<std::string, double>& map_words_next = search_server.GetWordFrequencies(*it2);
            const std::set<std::string> set_words2 = ConvertMapToSet(map_words_next);
            // найден дубль
            if (set_words1 == set_words2) {
                id_for_remove.insert(*it2);
            }
            
        }

    }

    for (const int id : id_for_remove) {
        search_server.RemoveDocument(id);
        std::cout << "Found duplicate document id " << id << std::endl;
    }
    
}