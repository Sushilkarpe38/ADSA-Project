#include "../include/ItemRecord.h"
#include "../include/Normalizer.h"
#include "../include/CategoryIndex.h"
#include "../include/Trie.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "====================================================\n";
    std::cout << "    ReFind: Phase 2 Normalizer, Index & Trie Test   \n";
    std::cout << "====================================================\n\n";

    // 1. Test Normalizer (M1)
    std::cout << "[*] 1. Initializing Normalizer (M1) ...\n";
    Normalizer normalizer;
    bool swLoaded = normalizer.loadStopWords("data/stopwords.txt");
    bool synLoaded = normalizer.loadSynonyms("data/synonyms.txt");
    std::cout << "    - Stopwords loaded: " << (swLoaded ? "YES" : "NO") << "\n";
    std::cout << "    - Synonyms loaded:  " << (synLoaded ? "YES" : "NO") << "\n";

    std::string testPhrase = "Lost my black mobile with a cracked screen please help";
    std::vector<std::string> tokens = normalizer.tokenizeAndNormalize(testPhrase);
    std::cout << "    - Input: \"" << testPhrase << "\"\n";
    std::cout << "    - Normalized tokens: [ ";
    for (const auto& t : tokens) std::cout << "\"" << t << "\" ";
    std::cout << "]\n";

    // Verify synonym "mobile" -> "phone"
    bool foundPhone = false;
    for (const auto& t : tokens) {
        if (t == "phone") foundPhone = true;
    }
    assert(foundPhone && "Synonym mapping 'mobile' -> 'phone' failed!");
    std::cout << "    [+] Synonym mapping verification passed: 'mobile' -> 'phone'\n\n";

    // 2. Load dataset
    std::string datasetPath = "data/reports_1k.csv";
    std::cout << "[*] 2. Loading and preparing items from " << datasetPath << " ...\n";
    std::vector<ItemRecord> items = ItemRecord::loadFromCSV(datasetPath);
    std::cout << "    - Loaded " << items.size() << " records.\n";

    for (auto& item : items) {
        normalizer.prepareItem(item);
    }
    std::cout << "    [+] Text normalized and tokenized for all " << items.size() << " records.\n\n";

    // 3. Test Category Index (M2)
    std::cout << "[*] 3. Testing Category Hash Map (M2) ...\n";
    CategoryIndex catIndex;
    for (const auto& item : items) {
        catIndex.insert(&item);
    }

    auto foundPhones = catIndex.lookup("phone", ReportType::FOUND);
    auto lostPhones = catIndex.lookup("phone", ReportType::LOST);
    std::cout << "    - FOUND phones bucket size: " << foundPhones.size() << "\n";
    std::cout << "    - LOST phones bucket size:  " << lostPhones.size() << "\n";
    assert(foundPhones.size() > 0 && "Category lookup for FOUND phones failed!");
    std::cout << "    [+] Category Hash Map verification passed.\n\n";

    // 4. Test Trie & Inverted Index (M3 Exact)
    std::cout << "[*] 4. Testing Trie & Inverted Index (M3) ...\n";
    Trie foundTrie;
    for (const auto& item : items) {
        if (item.type == ReportType::FOUND) {
            for (const auto& w : item.name_words) {
                foundTrie.insert(w, item.id);
            }
            for (const auto& w : item.description_words) {
                foundTrie.insert(w, item.id);
            }
        }
    }
    std::cout << "    - Total unique vocabulary words indexed in Trie: " << foundTrie.getWordCount() << "\n";

    std::vector<std::string> testWords = {"samsung", "iphone", "black", "screen", "macbook"};
    for (const auto& word : testWords) {
        int df = foundTrie.getDocFreq(word);
        const auto& postings = foundTrie.getPostings(word);
        std::cout << "    - Word: \"" << word << "\" | Document Frequency: " << df
                  << " | Postings Sample: [ ";
        for (size_t i = 0; i < std::min((size_t)4, postings.size()); ++i) {
            std::cout << postings[i] << " ";
        }
        if (postings.size() > 4) std::cout << "... ";
        std::cout << "]\n";
    }

    // Verify non-existent word
    int nonExistentDf = foundTrie.getDocFreq("nonexistentword123");
    assert(nonExistentDf == 0 && "Non-existent word check failed!");
    std::cout << "    [+] Non-existent word lookup correctly returned 0.\n";

    std::cout << "\n[+] Phase 2 validation completed successfully!\n";
    return 0;
}
