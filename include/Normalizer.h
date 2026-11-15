#ifndef NORMALIZER_H
#define NORMALIZER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "ItemRecord.h"

class Normalizer {
private:
    std::unordered_set<std::string> stopWords;
    std::unordered_map<std::string, std::string> synonyms;

    // Helper to strip non-alphanumeric chars and convert to lower
    std::string cleanToken(const std::string& raw) const;

public:
    Normalizer();

    bool loadStopWords(const std::string& filepath);
    bool loadSynonyms(const std::string& filepath);

    // Normalize free text: lowercase, strip punctuation, remove stopwords, map synonyms
    std::vector<std::string> tokenizeAndNormalize(const std::string& text) const;

    // Prepare item: normalize fields and populate name_words and description_words
    void prepareItem(ItemRecord& item) const;
};

#endif // NORMALIZER_H
