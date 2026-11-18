#include "../include/Normalizer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

Normalizer::Normalizer() {}

std::string Normalizer::cleanToken(const std::string& raw) const {
    std::string cleaned;
    for (char c : raw) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            cleaned += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return cleaned;
}

bool Normalizer::loadStopWords(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[Warning] Could not open stopwords file: " << filepath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::string word = cleanToken(line);
        if (!word.empty()) {
            stopWords.insert(word);
        }
    }
    return true;
}

bool Normalizer::loadSynonyms(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[Warning] Could not open synonyms file: " << filepath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        size_t comma = line.find(',');
        if (comma != std::string::npos) {
            std::string fromWord = cleanToken(line.substr(0, comma));
            std::string toWord = cleanToken(line.substr(comma + 1));
            if (!fromWord.empty() && !toWord.empty()) {
                synonyms[fromWord] = toWord;
            }
        }
    }
    return true;
}

std::vector<std::string> Normalizer::tokenizeAndNormalize(const std::string& text) const {
    std::vector<std::string> tokens;
    std::string current;

    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            current += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        } else {
            if (!current.empty()) {
                // Check if stopword
                if (stopWords.find(current) == stopWords.end()) {
                    // Check if synonym mapping exists
                    auto synIt = synonyms.find(current);
                    if (synIt != synonyms.end()) {
                        tokens.push_back(synIt->second);
                    } else {
                        tokens.push_back(current);
                    }
                }
                current.clear();
            }
        }
    }

    if (!current.empty()) {
        if (stopWords.find(current) == stopWords.end()) {
            auto synIt = synonyms.find(current);
            if (synIt != synonyms.end()) {
                tokens.push_back(synIt->second);
            } else {
                tokens.push_back(current);
            }
        }
    }

    return tokens;
}

void Normalizer::prepareItem(ItemRecord& item) const {
    // Normalize category (single word lowercase)
    item.category = cleanToken(item.category);

    // Normalize name into name_words
    item.name_words = tokenizeAndNormalize(item.name);

    // Normalize description into description_words
    item.description_words = tokenizeAndNormalize(item.description);

    // Tokenize color and location for fuzzy/partial Trie matching
    item.color_words    = tokenizeAndNormalize(item.color);
    item.location_words = tokenizeAndNormalize(item.location);

    // Keep raw color as the first token (for display) or leave as-is
    // The raw strings item.color / item.location are still kept for display
}
